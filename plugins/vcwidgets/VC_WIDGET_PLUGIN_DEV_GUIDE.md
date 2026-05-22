# VC Widget Plugin — Przewodnik dla developerów

> Wersja: 1.2 | QLC+ 4.14 / GRIDqlc | Qt 6 | macOS Apple Silicon

---

## Spis treści

1. [Architektura](#1-architektura)
2. [Struktura plików pluginu](#2-struktura-plików-pluginu)
3. [Interfejs pluginu — VCWidgetPluginInterface](#3-interfejs-pluginu--vcwidgetplugininterface)
4. [Widget — VCWidget + DMXSource](#4-widget--vcwidget--dmxsource)
5. [Kluczowe: zapis DMX — GenericFader, nie Universe::write()](#5-kluczowe-zapis-dmx--genericfader-nie-universewrite)
6. [Thread safety — co wolno gdzie](#6-thread-safety--co-wolno-gdzie)
7. [Tryby Design / Operate](#7-tryby-design--operate)
8. [External input — MIDI/OSC/DMX](#8-external-input--midioscdmx)
9. [Sterowanie funkcjami QLC+ — Function, FunctionParent, FunctionSelection](#9-sterowanie-funkcjami-qlc--function-functionparent-functionselection)
10. [Serializacja XML](#10-serializacja-xml)
11. [CMakeLists.txt pluginu](#11-cmakeliststxt-pluginu)
12. [Build na macOS Apple Silicon](#12-build-na-macos-apple-silicon)
13. [Pakowanie i instalacja (.qlcvcw)](#13-pakowanie-i-instalacja-qlcvcw)
14. [Pułapki i najczęstsze błędy](#14-pułapki-i-najczęstsze-błędy)

---

## 1. Architektura

```
QLC+ (qlcplusui + qlcplusengine)
│
├── VCWidgetPluginManager          ← skanuje katalogi, ładuje .dylib
│   └── load(QDir)                 ← filtr: *.dylib (macOS), *.so (Linux), *.dll (Win)
│
├── VirtualConsole                 ← menu Add → dynamicznie z załadowanych pluginów
│
└── Plugin .dylib
    ├── MojaKlasaPlugin            ← implementuje VCWidgetPluginInterface
    │   └── createWidget()         ← fabryka widgetu
    └── MojaKlasaWidget            ← dziedziczy VCWidget + DMXSource
        ├── editProperties()       ← dialog konfiguracji (tryb Design)
        ├── writeDMX()             ← wywołanie co tick (wątek MasterTimer)
        ├── slotModeChanged()      ← register/unregister DMXSource
        ├── loadXML() / saveXML()  ← serializacja projektu
        └── createCopy()           ← kopiowanie widgetu
```

### Cykl życia ticka MasterTimer (ważne!)

```
MasterTimer::timerTick()
  │
  ├── timerTickFunctions()         ← funkcje QLC+ (Scenes, Chasers…)
  │
  ├── timerTickDMXSources()        ← NASZ writeDMX() tutaj
  │   └── widget->writeDMX()
  │       └── fader->getChannelFader(...)->setTarget(val)
  │
  └── emit tickReady               ← QueuedConnection → wątek Universe
        │
        └── Universe::processFaders()
              ├── zeroIntensityChannels()   ← zeruje HTP kanały w preGM
              ├── fader->write(universe)    ← NASZ fader pisze po zerowaniu ✓
              └── dumpOutput()             → output plugin → DMX hardware
```

**Kluczowy wniosek:** `Universe::write()` woła się PRZED `zeroIntensityChannels()`, więc wartość jest kasowana. Zawsze używaj `GenericFader`.

---

## 2. Struktura plików pluginu

```
plugins/vcwidgets/myplugin/
├── CMakeLists.txt
├── mypluginplugin.h        ← VCWidgetPluginInterface
├── mypluginplugin.cpp
├── mypluginwidget.h        ← VCWidget + DMXSource
├── mypluginwidget.cpp
└── mypluginconfigdialog.h/.cpp   ← opcjonalny dialog właściwości
```

Przykład gotowy: `plugins/vcwidgets/dmxnumeric/`

---

## 3. Interfejs pluginu — VCWidgetPluginInterface

```cpp
// mypluginplugin.h
#include "vcwidgetplugininterface.h"   // ui/src/virtualconsole/

class MyPlugin : public QObject, public VCWidgetPluginInterface
{
    Q_OBJECT
    Q_INTERFACES(VCWidgetPluginInterface)
    Q_PLUGIN_METADATA(IID VCWidgetPlugin_iid FILE "myplugin.json")

public:
    QString pluginId()    const override { return "org.qlcplus.vcwidgets.myplugin"; }
    QString name()        const override { return "My Plugin"; }
    QString version()     const override { return "1.0.0"; }
    QString author()      const override { return "Autor"; }
    QString description() const override { return "Opis..."; }
    QString category()    const override { return "DMX Control"; }
    QIcon   icon()        const override { return QIcon(); }

    VCWidget* createWidget(QWidget* parent, Doc* doc) override
    {
        return new MyWidget(parent, doc);
    }
};
```

```json
// myplugin.json — metadane Qt plugin (wymagane przez Q_PLUGIN_METADATA)
{ "Keys": [] }
```

> **Plugin ID musi być globalnie unikalny** — używaj reverse-domain style:
> `org.twojadomena.vcwidgets.nazwaplugin`

---

## 4. Widget — VCWidget + DMXSource

```cpp
// mypluginwidget.h
#include "vcwidget.h"
#include "dmxsource.h"
#include "genericfader.h"  // engine/src/

class MyWidget : public VCWidget, public DMXSource
{
    Q_OBJECT

public:
    MyWidget(QWidget* parent, Doc* doc);
    ~MyWidget() override;

    // VCWidget — wymagane
    VCWidget* createCopy(VCWidget* parent) override;
    void      updateFeedback() override {}
    bool      loadXML(QXmlStreamReader& root) override;
    bool      saveXML(QXmlStreamWriter* doc) override;
    void      editProperties() override;

    // DMXSource — wymagane
    void writeDMX(MasterTimer* timer, QList<Universe*> universes) override;

protected slots:
    void slotModeChanged(Doc::Mode mode) override;

protected:
    void paintEvent(QPaintEvent* e) override;

private:
    // Konfiguracja (fixture-based — nie raw address!)
    quint32 m_fixtureId = UINT_MAX;   // Fixture::invalidId()
    quint32 m_channel   = 0;

    // Wartość DMX — współdzielona między wątkiem GUI i timerem
    QMutex m_valueMutex;
    uchar  m_dmxValue = 0;

    // Fader — jedyne poprawne API do zapisu DMX
    QSharedPointer<GenericFader> m_fader;

    // UI…
};
```

---

## 5. Kluczowe: zapis DMX — GenericFader, nie Universe::write()

### ❌ ŹLE — Universe::write() bezpośrednio

```cpp
void MyWidget::writeDMX(MasterTimer*, QList<Universe*> universes)
{
    // To jest ZEROWANE przez zeroIntensityChannels() przed dumpOutput!
    universes[0]->write(42, m_dmxValue, true);  // ← output zawsze 0
}
```

### ✅ DOBRZE — GenericFader + FadeChannel

```cpp
void MyWidget::writeDMX(MasterTimer*, QList<Universe*> universes)
{
    QMutexLocker lk(&m_valueMutex);   // ← mutex bo wołane z timera!

    Fixture* fxi = m_doc->fixture(m_fixtureId);
    if (!fxi || m_channel >= fxi->channels())
        return;

    quint32 uni = fxi->universe();
    if ((int)uni >= universes.size())
        return;

    // requestFader() TYLKO przy pierwszym ticku (lub po reset)
    if (m_fader.isNull())
        m_fader = universes[uni]->requestFader(Universe::Auto);

    FadeChannel* fc = m_fader->getChannelFader(
        m_doc, universes[uni], m_fixtureId, m_channel);

    if (fc->universe() == Universe::invalid())
    {
        m_fader->remove(fc);
        return;
    }

    // Ustaw wartość docelową — fader zapisze ją po zeroIntensityChannels()
    fc->setStart(fc->current());
    fc->setTarget(m_dmxValue);
    fc->setReady(false);
    fc->setElapsed(0);
}
```

### Dlaczego fixture + channel zamiast universe + address?

`GenericFader::getChannelFader()` potrzebuje ID fixture'a i numeru kanału wewnątrz fixture'a. QLC+ używa tej pary do poprawnego obsługi HTP/LTP, modyfikatorów kanałów, Grand Mastera i monitora DMX.

Konwersja jest prosta: `absoluteAddress = fixture->address() + channel`.

---

## 6. Thread safety — co wolno gdzie

| Kontekst | Wątek | Co można |
|---|---|---|
| Konstruktor, `editProperties`, `paintEvent`, sloty Qt | GUI thread | Dostęp do wszystkich Qt UI, `m_doc`, `Fixture` |
| `writeDMX(MasterTimer*, universes)` | MasterTimer thread | **NIE dotykać UI!** Tylko dane przez mutex |
| `slotModeChanged` | GUI thread | `registerDMXSource` / `unregisterDMXSource`, reset UI |

**Zasada:** wszystkie pola czytane/pisane w `writeDMX` muszą być chronione przez `QMutex`:

```cpp
// GUI thread — np. po zmianie spinboxa:
void MyWidget::slotValueChanged(int v)
{
    QMutexLocker lk(&m_valueMutex);
    m_dmxValue = uchar(v);
}

// Timer thread:
void MyWidget::writeDMX(MasterTimer*, QList<Universe*> universes)
{
    QMutexLocker lk(&m_valueMutex);
    // bezpieczne odczytanie m_dmxValue
}
```

---

## 7. Tryby Design / Operate

```cpp
void MyWidget::slotModeChanged(Doc::Mode mode)
{
    if (mode == Doc::Operate)
    {
        m_spinBox->setEnabled(true);
        m_doc->masterTimer()->registerDMXSource(this);   // ← start writeDMX ticks
    }
    else  // Design
    {
        m_spinBox->setEnabled(false);
        m_doc->masterTimer()->unregisterDMXSource(this); // ← stop ticks

        // Zwolnij fader — universe przestaje trzymać nasze wartości
        if (!m_fader.isNull())
        {
            m_fader->requestDelete();  // bezpieczne usunięcie z wątku Universe
            m_fader.clear();
        }
    }
    VCWidget::slotModeChanged(mode);   // ← zawsze wywołaj base class!
    update();
}
```

**Destruktor** — zawsze wyrejestruj i zwolnij fader:

```cpp
MyWidget::~MyWidget()
{
    if (m_doc && m_doc->masterTimer())
        m_doc->masterTimer()->unregisterDMXSource(this);
    if (!m_fader.isNull())
        m_fader->requestDelete();
}
```

---

## 8. External input — MIDI/OSC/DMX

QLC+ udostępnia system external input pozwalający mapować sygnały MIDI, OSC, HID lub DMX na dowolne kontrolki w widgetach. Mechanizm jest zaimplementowany w `VCWidget` — plugin dziedziczy go bezpłatnie.

### Koncepcja

Każdy widget może mieć wiele **input source'ów**, rozróżnianych przez `quint8 sourceId`. Daje to możliwość przypisania różnych sygnałów do różnych akcji (np. "ustaw wartość" vs "wyzwól Apply").

```
MIDI suwak → slotInputValueChanged(universe, channel, value)
                  └── checkInputSource(universe, channel, value, sender(), valueInputSourceId)
                            → m_spinBox->setValue(value)

MIDI button → slotInputValueChanged(universe, channel, value)
                  └── checkInputSource(universe, channel, value, sender(), applyInputSourceId)
                            → slotApply()   (jeśli value > 0)
```

### Definicja ID inputów

```cpp
// W nagłówku widgetu:
static const quint8 valueInputSourceId = 0;
static const quint8 applyInputSourceId = 1;
```

### Override slotInputValueChanged

```cpp
void MyWidget::slotInputValueChanged(quint32 universe, quint32 channel, uchar value)
{
    if (acceptsInput() == false)
        return;

    quint32 pagedCh = (page() << 16) | channel;   // obsługa stron (pages)

    if (checkInputSource(universe, pagedCh, value, sender(), valueInputSourceId))
    {
        // Mapowanie wartości (0–255 MIDI → 0–255 DMX, lub przeskaluj jeśli inny zakres)
        m_spinBox->setValue(int(value));
    }
    else if (checkInputSource(universe, pagedCh, value, sender(), applyInputSourceId))
    {
        if (value > 0)        // dowolna wartość niezerowa = trigger
            slotApply();
    }
}
```

### Feedback do MIDI (motorized faders, LED)

```cpp
void MyWidget::updateFeedback()
{
    // Wywoływane przez QLC+ przy zmianie strony / wyświetleniu widgetu
    QMutexLocker lk(&m_valueMutex);
    sendFeedback(int(m_dmxValue), valueInputSourceId);
}

void MyWidget::slotApply()
{
    {
        QMutexLocker lk(&m_valueMutex);
        m_dmxValue = uchar(m_spinBox->value());
    }
    // Wyślij aktualną wartość z powrotem do kontrolera (LED / motorized fader)
    sendFeedback(m_spinBox->value(), valueInputSourceId);
}
```

### Kopiowanie input source'ów (createCopy)

```cpp
VCWidget* MyWidget::createCopy(VCWidget* parent)
{
    MyWidget* copy = new MyWidget(parent, m_doc);
    copy->copyFrom(this);
    copy->setInputSource(inputSource(valueInputSourceId), valueInputSourceId);
    copy->setInputSource(inputSource(applyInputSourceId), applyInputSourceId);
    return copy;
}
```

### Dialog konfiguracji — InputSelectionWidget

Użyj `InputSelectionWidget` żeby użytkownik mógł mapować MIDI/OSC/DMX przez Auto Detect:

```cpp
#include "inputselectionwidget.h"

// W konstruktorze dialogu konfiguracji:
m_valueInputSel = new InputSelectionWidget(doc, this);
m_valueInputSel->setKeyInputVisibility(false);   // nie potrzebujemy klawiatury
m_valueInputSel->setWidgetPage(widgetPage);      // dla obsługi stron
m_valueInputSel->setInputSource(valueSrc);       // bieżące mapowanie (może być null)

// Po accept:
auto src = m_valueInputSel->inputSource();
widget->setInputSource(src, MyWidget::valueInputSourceId);
```

Dla dialogu przyjmuj źródła przez konstruktor, a zwracaj przez gettery:

```cpp
// Konstruktor dialogu (preferuj przekazywanie przez wartość, nie przez widget*)
explicit MyConfigDialog(Doc* doc,
                        quint32 currentFxId, quint32 currentChannel,
                        QSharedPointer<QLCInputSource> valueSrc,
                        QSharedPointer<QLCInputSource> applySrc,
                        int widgetPage,
                        QWidget* parent = nullptr);

QSharedPointer<QLCInputSource> valueInputSource() const;
QSharedPointer<QLCInputSource> applyInputSource()  const;
```

### XML — serializacja input bindingów

Wzorzec z zagnieżdżonym `<Input>` (taki sam jak `VCSpeedDial`):

```xml
<ValueInput>
  <Input Universe="0" Channel="42"/>
</ValueInput>
<ApplyInput>
  <Input Universe="0" Channel="43"/>
</ApplyInput>
```

**saveXML:**

```cpp
auto valueSrc = inputSource(valueInputSourceId);
if (!valueSrc.isNull() && valueSrc->isValid())
{
    doc->writeStartElement("ValueInput");
    saveXMLInput(doc, valueSrc);   // zapisuje <Input Universe=... Channel=.../>
    doc->writeEndElement();
}
// analogicznie ApplyInput
```

**loadXML:**

```cpp
else if (root.name() == "ValueInput")
    loadXMLSources(root, valueInputSourceId);   // parsuje <Input/> i woła setInputSource
else if (root.name() == "ApplyInput")
    loadXMLSources(root, applyInputSourceId);
```

> `loadXMLSources` / `saveXMLInput` są metodami `VCWidget` — dziedziczysz je za darmo.

---

## 9. Sterowanie funkcjami QLC+ — Function, FunctionParent, FunctionSelection

Jesli widget nie pisze DMX bezposrednio, ale **startuje i zatrzymuje funkcje** QLC+ (Sceny, Chasery, EFX itp.), implementacja jest inna niz DMXSource. Nie potrzebujesz `GenericFader` ani `DMXSource` — wystarczy `VCWidget`.

### Wymagana metoda: `functionParent()`

Kazdy widget ktory uzywa `Function::start()` musi dostarczyc obiekt `FunctionParent` identyfikujacy kto jest "wlascicielem" wywolania. Bez tego QLC+ nie moze prawidlowo zatrzymac funkcji przy np. SoloFrame.

```cpp
// Naglowek widgetu:
#include "functionparent.h"
FunctionParent functionParent() const;

// Implementacja (zawsze identyczna dla widget plugins):
FunctionParent MyWidget::functionParent() const
{
    return FunctionParent(FunctionParent::ManualVCWidget, id());
}
```

### Start i Stop funkcji

```cpp
#include "function.h"
#include "mastertimer.h"

// Start:
Function* f = m_doc->function(functionId);
if (f)
    f->start(m_doc->masterTimer(), functionParent());

// Stop:
if (f)
    f->stop(functionParent());
```

> **Wazne:** zawsze zatrzymuj aktywna funkcje w `slotModeChanged(Design)` i w destruktorze. Niezatrzymana funkcja dziala w tle nawet po usunieciu widgetu z projektu.

### Wybor funkcji przez uzytkownika — FunctionSelection

Standardowy dialog wyboru funkcji z QLC+, ten sam co w przyciskach, sliderach itp.:

```cpp
#include "functionselection.h"

// Wlaczenie w CMakeLists.txt: nic dodatkowego — jest w libqlcplusui

// Jednorazowy wybor:
FunctionSelection fs(parentWidget, m_doc);
fs.setMultiSelection(false);
if (fs.exec() == QDialog::Accepted)
{
    QList<quint32> selected = fs.selection();
    if (!selected.isEmpty())
        m_functionId = selected.first();
}

// Wielokrotny wybor (np. lista funkcji do kolejkowania):
FunctionSelection fs(parentWidget, m_doc);
fs.setMultiSelection(true);
if (fs.exec() == QDialog::Accepted)
{
    for (quint32 id : fs.selection())
        m_functionIds.append(id);
}
```

### Wzorzec: Widget sterujacy funkcjami (bez DMX)

```cpp
// mywidget.h — NIE dziedziczy DMXSource, NIE implementuje writeDMX
class MyWidget : public VCWidget
{
    Q_OBJECT
public:
    FunctionParent functionParent() const;
    // ...
protected slots:
    void slotModeChanged(Doc::Mode mode) override;
private:
    quint32 m_functionId = Function::invalidId();
};

// mywidget.cpp
FunctionParent MyWidget::functionParent() const
{
    return FunctionParent(FunctionParent::ManualVCWidget, id());
}

void MyWidget::slotModeChanged(Doc::Mode mode)
{
    if (mode == Doc::Design)
    {
        Function* f = m_doc->function(m_functionId);
        if (f) f->stop(functionParent());
    }
    VCWidget::slotModeChanged(mode);
}

MyWidget::~MyWidget()
{
    Function* f = m_doc->function(m_functionId);
    if (f) f->stop(functionParent());
    // Brak unregisterDMXSource — nie jestesmy DMXSource
}
```

### Popup menu dla szybkiego wyboru funkcji w Operate

Przydatny wzorzec dla widgetow z lista funkcji — popup aktywowany long-pressem lub prawym klikiem:

```cpp
void MyWidget::contextMenuEvent(QContextMenuEvent* e)
{
    if (mode() == Doc::Operate)
    {
        QMenu menu(this);
        for (int i = 0; i < m_functionIds.size(); ++i)
        {
            Function* f = m_doc->function(m_functionIds.at(i));
            QAction* act = menu.addAction(f ? f->name() : tr("?"));
            act->setCheckable(true);
            act->setChecked(i == m_currentIndex);
            act->setData(i);
        }
        QAction* chosen = menu.exec(e->globalPos());
        if (chosen) activate(chosen->data().toInt());
        e->accept();
        return;
    }
    VCWidget::contextMenuEvent(e);   // Design: standardowe menu
}
```

### Long-press (touch-friendly)

```cpp
// W konstruktorze:
m_longPressTimer = new QTimer(this);
m_longPressTimer->setSingleShot(true);
connect(m_longPressTimer, &QTimer::timeout, this, &MyWidget::slotLongPressFired);

// mousePressEvent:
if (mode() == Doc::Operate && e->button() == Qt::LeftButton)
{
    m_pressActive = true;
    m_longFired   = false;
    m_pressPos    = e->pos();
    m_longPressTimer->start(m_longPressMs);   // np. 500 ms
    e->accept(); return;
}

// mouseReleaseEvent:
if (m_pressActive && e->button() == Qt::LeftButton)
{
    m_pressActive = false;
    m_longPressTimer->stop();
    if (!m_longFired && rect().contains(e->pos()))
        doShortPress();   // krotki klik
    e->accept(); return;
}

// mouseMoveEvent — anuluj timer przy ruchu:
if (m_pressActive)
{
    if ((e->pos() - m_pressPos).manhattanLength() > 10)
    {
        m_longPressTimer->stop();
        m_longFired = true;
    }
    e->accept(); return;
}
```

---

## 10. Serializacja XML


### Format zapisu (nowy)

```xml
<PluginWidget PluginId="org.qlcplus.vcwidgets.myplugin">
  <Caption>My Widget</Caption>
  <WindowState visible="True" x="10" y="10" width="140" height="80"/>
  <Appearance>…</Appearance>
  <FixtureID>3</FixtureID>
  <Channel>0</Channel>
  <Value>200</Value>
</PluginWidget>
```

### Implementacja

```cpp
bool MyWidget::loadXML(QXmlStreamReader& root)
{
    if (root.name() != QLatin1String("PluginWidget")) return false;

    loadXMLCommon(root);         // Caption, PluginId attr

    while (root.readNextStartElement())
    {
        if (root.name() == KXMLQLCWindowState)
        {
            int x,y,w,h; bool vis;
            loadXMLWindowState(root, &x,&y,&w,&h,&vis);
            setGeometry(x,y,w,h);
        }
        else if (root.name() == KXMLQLCVCWidgetAppearance)
            loadXMLAppearance(root);
        else if (root.name() == "FixtureID")
            m_fixtureId = root.readElementText().toUInt();
        else if (root.name() == "Channel")
            m_channel = root.readElementText().toUInt();
        else if (root.name() == "Value")
            m_dmxValue = uchar(root.readElementText().toInt());
        else
            root.skipCurrentElement();   // ← ZAWSZE obsłuż nieznane tagi!
    }
    return true;
}

bool MyWidget::saveXML(QXmlStreamWriter* doc)
{
    doc->writeStartElement("PluginWidget");
    doc->writeAttribute("PluginId", "org.qlcplus.vcwidgets.myplugin");

    saveXMLCommon(doc);
    saveXMLWindowState(doc);
    saveXMLAppearance(doc);

    doc->writeTextElement("FixtureID", QString::number(m_fixtureId));
    doc->writeTextElement("Channel",   QString::number(m_channel));
    doc->writeTextElement("Value",     QString::number(m_dmxValue));

    doc->writeEndElement();
    return true;
}
```

> **Ważne:** zawsze przechowuj dane jako `FixtureID + Channel`, nie `Universe + Address`. Adresy fixtur mogą się zmieniać po re-patchowaniu.

---

## 11. CMakeLists.txt pluginu

```cmake
cmake_minimum_required(VERSION 3.16)
project(myplugin_vcwidget)

set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

if(NOT DEFINED QLCPLUS_SRC_DIR OR NOT DEFINED QLCPLUS_BUILD_DIR)
    message(FATAL_ERROR "Podaj -DQLCPLUS_SRC_DIR= i -DQLCPLUS_BUILD_DIR=")
endif()

find_library(QLCPLUSUI_LIB     qlcplusui     HINTS "${QLCPLUS_BUILD_DIR}/ui/src"     REQUIRED)
find_library(QLCPLUSENGINE_LIB qlcplusengine HINTS "${QLCPLUS_BUILD_DIR}/engine/src" REQUIRED)

add_library(myplugin_vcwidget MODULE
    mypluginplugin.cpp mypluginplugin.h
    mypluginwidget.cpp mypluginwidget.h
)

target_compile_features(myplugin_vcwidget PRIVATE cxx_std_17)

target_link_libraries(myplugin_vcwidget PRIVATE
    Qt6::Core Qt6::Widgets
    ${QLCPLUSUI_LIB}
    ${QLCPLUSENGINE_LIB}
)

target_include_directories(myplugin_vcwidget PRIVATE
    "${QLCPLUS_SRC_DIR}/ui/src/virtualconsole"   # VCWidget, VCWidgetPluginInterface
    "${QLCPLUS_SRC_DIR}/engine/src"              # Fixture, Doc, Universe, GenericFader…
)

# KRYTYCZNE na macOS: QLC+ filtruje *.dylib, nie *.so
# Bez tego plugin istnieje jako .so ale jest niewidoczny dla managera!
if(APPLE)
    set_target_properties(myplugin_vcwidget PROPERTIES SUFFIX ".dylib")
endif()
```

---

## 12. Build na macOS Apple Silicon

### Jednorazowe przygotowanie

```bash
# Znajdź Qt używany przez Twój build QLC+:
grep CMAKE_PREFIX_PATH /path/to/qlcplus/build/CMakeCache.txt
# → /opt/homebrew/lib/cmake   (użyj tego w cmake poniżej)
```

### Build pluginu

```bash
cd plugins/vcwidgets/myplugin
rm -rf build && mkdir build && cd build

cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DQLCPLUS_SRC_DIR="/path/to/qlcplus" \
  -DQLCPLUS_BUILD_DIR="/path/to/qlcplus/build" \
  -DCMAKE_PREFIX_PATH="/opt/homebrew/lib/cmake" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0

cmake --build .
```

### Naprawa rpath i codesign (WYMAGANE na macOS)

Po każdym buildzie:

```bash
PLUGIN="./libmyplugin_vcwidget.dylib"
BUNDLE="$HOME/QLC+.app"

# Dodaj rpath do bundlu QLC+ (żeby plugin znalazł libqlcplusui.dylib)
install_name_tool -add_rpath "$BUNDLE/Contents/Frameworks" "$PLUGIN" 2>/dev/null || true

# Napraw ścieżki Qt do zgodności z qt.conf bundlu
for qtfw in QtCore QtGui QtWidgets; do
  install_name_tool -change \
    "/opt/homebrew/opt/qtbase/lib/$qtfw.framework/Versions/A/$qtfw" \
    "/opt/homebrew/lib/$qtfw.framework/Versions/A/$qtfw" \
    "$PLUGIN" 2>/dev/null || true
done

# Podpisz (macOS wymaga po modyfikacji binariów)
codesign --force --sign - "$PLUGIN"
```

### Instalacja do katalogu użytkownika

```bash
DEST="$HOME/Library/Application Support/QLC+/VCWidgets"
mkdir -p "$DEST"
cp "$PLUGIN" "$DEST/"
```

Uruchom QLC+ ponownie — plugin załaduje się automatycznie.

---

## 13. Pakowanie i instalacja (.qlcvcw)

Plik `.qlcvcw` to zwykły ZIP z `manifest.json` i biblioteką.

### manifest.json

> **Krytyczne:** instalator (`VCWidgetPluginInstaller`) waliduje manifest i wymaga pól `plugin_id` oraz `platforms`. Inne pola są opcjonalne. Pole `platforms` to **obiekt** (nie tablica), gdzie klucz to identyfikator platformy, a wartość to nazwa pliku binarnego.

Klucze platform rozpoznawane przez QLC+:

| Klucz | Platforma |
|---|---|
| `macos_arm64` | macOS Apple Silicon |
| `macos_x64`   | macOS Intel |
| `linux_x64`   | Linux 64-bit |
| `windows_x64` | Windows 64-bit |

```json
{
  "plugin_id":  "org.qlcplus.vcwidgets.myplugin",
  "name":       "My Plugin",
  "version":    "1.0.0",
  "author":     "Imię Nazwisko",
  "description": "Co robi plugin.",
  "platforms": {
    "macos_arm64": "libmyplugin_vcwidget.dylib",
    "macos_x64":   "libmyplugin_vcwidget.dylib",
    "linux_x64":   "libmyplugin_vcwidget.so",
    "windows_x64": "myplugin_vcwidget.dll"
  }
}
```

### Tworzenie paczki (Python)

```python
import os, shutil, zipfile, json, tempfile

src  = "build/libmyplugin_vcwidget.dylib"
work = tempfile.mkdtemp()

# manifest
with open(f"{work}/manifest.json", "w") as f:
    json.dump(manifest, f, indent=2)    # manifest dict jak wyżej

shutil.copy(src, f"{work}/libmyplugin_vcwidget.dylib")

with zipfile.ZipFile("myplugin-1.0.0.qlcvcw", "w", zipfile.ZIP_DEFLATED) as zf:
    zf.write(f"{work}/manifest.json", "manifest.json")
    zf.write(f"{work}/libmyplugin_vcwidget.dylib", "libmyplugin_vcwidget.dylib")
```

### Instalacja przez UI QLC+

**Virtual Console → Add → Get more widgets... → Install from file...** → wybierz `.qlcvcw`.

Instalator wypakowuje plik do katalogu tymczasowego, czyta `manifest.json`, wybiera binarię dla bieżącej platformy i kopiuje do `~/Library/Application Support/QLC+/VCWidgets/`.

---

## 14. Pułapki i najczęstsze błędy

### ❌ `Universe::write()` zamiast `GenericFader`

**Objaw:** output DMX zawsze 0, mimo że widget wyświetla wartość.  
**Przyczyna:** `zeroIntensityChannels()` zeruje wynik `write()` przed `dumpOutput()`.  
**Fix:** użyj `requestFader()` + `getChannelFader()` + `fc->setTarget()`.

---

### ❌ Plugin jako `.so` zamiast `.dylib` na macOS

**Objaw:** plugin zbudowany, plik istnieje w `VCWidgets/`, ale nie pojawia się na liście.  
**Przyczyna:** `VCWidgetPluginManager::load()` filtruje wg `KExtPlugin = ".dylib"` (macOS).  
**Fix:** w `CMakeLists.txt`:
```cmake
if(APPLE)
    set_target_properties(myplugin PROPERTIES SUFFIX ".dylib")
endif()
```

---

### ❌ Brak `rpath` do bundlu QLC+

**Objaw:** `QPluginLoader` zwraca błąd `image not found` lub `incompatible`.  
**Przyczyna:** plugin szuka `@rpath/libqlcplusui.dylib` ale `@rpath` wskazuje tylko na katalog build.  
**Fix:** `install_name_tool -add_rpath "$HOME/QLC+.app/Contents/Frameworks" plugin.dylib`

---

### ❌ Qt build z innym prefixem niż QLC+

**Objaw:** `"built for a different Qt version or compiler"`.  
**Diagnoza:** `grep CMAKE_PREFIX_PATH qlcplus/build/CMakeCache.txt` → użyj DOKŁADNIE tego samego prefixu.  
**Fix:** `-DCMAKE_PREFIX_PATH="/opt/homebrew/lib/cmake"` (nie `qt@5`, nie `qt@6` — sprawdź cache!).

---

### ❌ Stara wersja w `/Applications/QLC+.app`

**Objaw:** zmiany w kodzie nie mają efektu, UI bez nowych funkcji.  
**Przyczyna:** macOS otwiera `/Applications/QLC+.app` (stara wersja) zamiast `~/QLC+.app`.  
**Fix:** zawsze uruchamiaj `open ~/QLC+.app` lub zastąp `/Applications/QLC+.app` ręcznie.

---

### ❌ Dotykanie UI w `writeDMX()`

**Objaw:** crash / undefined behavior.  
**Przyczyna:** `writeDMX` wywoływane z wątku MasterTimer, UI tylko z GUI thread.  
**Fix:** w `writeDMX` tylko odczyt/zapis danych przez `QMutexLocker`. Żadnych `setText`, `setValue`, `update()`.

---

### ❌ Błędna struktura manifest.json

**Objaw:** instalator pokazuje `"manifest.json is missing required fields (plugin_id, platforms)"`.  
**Przyczyna:** użycie camelCase (`pluginId`) zamiast `plugin_id`, lub `platforms` jako tablica obiektów zamiast płaskiego obiektu klucz→plik.  
**Fix:** sprawdź dokładnie nazwy pól:

```json
// ❌ ŹLE — camelCase i tablica
{ "pluginId": "...", "binaries": [{ "platform": "macos_arm64", "file": "lib.dylib" }] }

// ✅ DOBRZE — snake_case i obiekt
{ "plugin_id": "...", "platforms": { "macos_arm64": "lib.dylib" } }
```

---

### ❌ Brak `root.skipCurrentElement()` w `loadXML`

**Objaw:** projekt nie ładuje się poprawnie, widgety gubią ustawienia.  
**Przyczyna:** `QXmlStreamReader` traci pozycję gdy nieznany tag nie jest konsumowany.  
**Fix:** zawsze w `else` bloku `while(readNextStartElement)` daj `root.skipCurrentElement()`.

---

### ❌ Brak `functionParent()` przy używaniu Function::start/stop

**Objaw:** funkcja startuje, ale nie da się jej zatrzymać przez SoloFrame; crash lub undefined behavior przy usuwaniu widgetu.  
**Przyczyna:** `Function::start()` i `stop()` wymagają `FunctionParent` identyfikujacego właściciela. Bez tego QLC+ nie wie komu "przypisać" instancję funkcji.  
**Fix:** zaimplementuj `FunctionParent functionParent() const` w widgecie:
```cpp
FunctionParent MyWidget::functionParent() const
{
    return FunctionParent(FunctionParent::ManualVCWidget, id());
}
```
I zawsze wywołaj `f->stop(functionParent())` w destruktorze oraz `slotModeChanged(Design)`.

---

### ❌ Niezatrzymana funkcja po wyjściu z Operate

**Objaw:** scena gra dalej mimo wyjścia z trybu Operate lub usunięcia widgetu.  
**Fix:** `slotModeChanged(Design)` musi wywołać `stopCurrent()`. Destruktor również.

---

### ❌ Brak `VCWidget::toClipboardJson(obj, doc)` w overridzie

Jeśli nadpisujesz `toClipboardJson` i zapomnisz zawołać bazy, JSON nie będzie zawierał `pluginId` — widget nie zostanie odtworzony przy wklejaniu.

```cpp
// ŹLE — brakuje wywołania bazy
void MyWidget::toClipboardJson(QJsonObject &obj, const Doc *doc) const
{
    obj["myState"] = m_myState;  // pluginId NIE zostanie zapisany!
}

// DOBRZE
void MyWidget::toClipboardJson(QJsonObject &obj, const Doc *doc) const
{
    VCWidget::toClipboardJson(obj, doc);   // najpierw baza — pluginId, geometry, caption, appearance, inputs
    obj["myState"] = m_myState;
}
```

To samo dotyczy `fromClipboardJson` — zawsze zawołaj `VCWidget::fromClipboardJson(obj, doc)` na początku.

### ❌ `slots` jako nazwa parametru lub zmiennej lokalnej

**Objaw:** błąd kompilacji `expected body of lambda expression` przy dostępie do `slots[i].field`.  
**Przyczyna:** `slots` jest makrem Qt (rozwinięcie `Q_SLOTS`). Użycie jako nazwy parametru koliduje z preprocesorem.  
**Fix:** używaj `initSlots`, `slotList`, `entries` lub innej nazwy — nigdy samego `slots`.

---

### ❌ `requestFader()` wywoływane co tick

**Objaw:** wycieki pamięci, konflikty faderów.  
**Przyczyna:** każde `requestFader()` tworzy NOWY fader w universe.  
**Fix:** sprawdzaj `if (m_fader.isNull())` przed `requestFader()`. Fader tworzy się raz, kasuje przy wyjściu z Operate.

---

## Minimalny gotowy przykład

Pełny przykład z poprawną implementacją: [`plugins/vcwidgets/dmxnumeric/`](../plugins/vcwidgets/dmxnumeric/)

Zawiera:
- `dmxnumericplugin.h/.cpp` — implementacja interfejsu
- `dmxnumericwidget.h/.cpp` — widget z GenericFader
- `dmxnumericconfigdialog.h/.cpp` — dialog wyboru fixture + kanał + InputSelectionWidget × 2
- `CMakeLists.txt` — gotowy do użycia
- `dmxnumeric-1.3.0.qlcvcw` — gotowa paczka do instalacji (v1.3: Apply + external input)

Drugi przykład, bardziej zaawansowany: [`plugins/vcwidgets/infinityencoder/`](../plugins/vcwidgets/infinityencoder/)

- `infinityencoderwidget.h/.cpp` — widget z 4 bankami kanałów (A/B/C/D), trybem relative (infinity encoder), animowaną galką, `QHash<quint32, QSharedPointer<GenericFader>>` per universe, 3 poziomami czułości
- `infinityencoderconfigdialog.h/.cpp` — dialog z 4 wierszami fixture/channel + 5× `InputSelectionWidget` (encoder + bank A/B/C/D)
- `infinityencoder-1.0.1.qlcvcw` — gotowa paczka (v1.0.1: circular drag interaction)

Trzeci przykład — widget sterujący **funkcjami** QLC+ (nie DMX bezpośrednio): [`plugins/vcwidgets/multibutton/`](../plugins/vcwidgets/multibutton/)

- Nie dziedziczy `DMXSource`, nie używa `GenericFader` — zamiast tego `Function::start/stop` z `FunctionParent`
- `multibuttonwidget.h/.cpp` — cyclic cycle (radio + wrap), long-press / right-click popup, kropkowe wskaźniki pozycji, QTimer dla long-press, `FunctionSelection` z `setMultiSelection(true)`
- `multibuttonconfigdialog.h/.cpp` — `QListWidget` z drag-drop reorder, Add/Remove/Edit label, `FunctionSelection`, `QSpinBox` dla long-press threshold, 2× `InputSelectionWidget`
- `multibutton-1.2.0.qlcvcw` — gotowa paczka (v1.2.0)

**Dodane w v1.1 / v1.2:**
- **OFF step** — opcjonalny krok OFF na końcu cyklu (checkbox `Add "OFF" step at end of cycle`); dot-wskaźnik OFF-a rysowany jako kwadracik zamiast kółka
- **Monitor channel values** — jak `VCButton::monitorChannelValues()`: timer 200 ms porównuje wartości DMX wyjścia z cached `SceneValue` każdego wpisu; gdy Scene innego widgetu pasuje, `m_currentIndex` aktualizuje się wizualnie (bez startowania funkcji); flaga `m_visualOnly` odróżnia stan monitorowany od aktywowanego przez użytkownika; amber-dot w prawym górnym rogu sygnalizuje monitored state
- **Per-entry Scribble/icon** — każdy wpis może mieć własną ikonę (plik lub Scribble); ikona aktywnego wpisu wyświetlana na buttonie; w Design-mode prawy klik → podmenu *Entry icon* (Scribble/Choose/Reset) z wyborem wpisu; w config dialogu przyciski Scribble/Choose/Clear icon z miniaturą w liście; path normalizowany jak w VCButton (filename-only dla `USERSCRIBBLEDIR`)
- **Opcjonalny tytuł** — wiersz tytułu pokazywany tylko gdy `caption()` nie jest pusty; gdy pusty — dynamiczna nazwa funkcji dostaje całą wysokość
- Popup menu (operate mode) pokazuje miniatury ikon obok nazw scen

---

---

## 15. Developer Mode — hot-reload bez restartu

### Folder użytkownika

Wszystkie user data (fixtures, scribbles, pluginy VC) są trzymane w jednym katalogu:

| System | Folder użytkownika |
|---|---|
| macOS | `~/Library/Application Support/QLC+/VCWidgets/` |
| Linux | `~/.qlcplus/vcwidgets/` |
| Windows | `%UserProfile%\QLC+\VCWidgets\` |

**Automatyczne ładowanie:** QLC+ monitoruje ten folder. Wrzucenie `.qlcvcw` lub `.dylib`/`.so`/`.dll` **automatycznie ładuje plugin** (bez restartu). Wystarczy skopiować plik.

### Developer Mode — workflow dla autorów pluginów

Włącz Developer Mode w: **Virtual Console → Get more widgets... → Settings → Developer mode**.

W Developer Mode:
1. Budujesz plugin: `cmake --build build`
2. Kopiujesz binarię do folderu użytkownika (jedno polecenie, np. skrypt)
3. QLC+ **automatycznie przeładowuje plugin** po wykryciu zmiany pliku
4. Istniejące instancje widgetu w Virtual Console są **serializowane do XML → plugin unload → reload → odtworzenie z XML**
5. Widget wraca dokładnie w to samo miejsce z tymi samymi ustawieniami

### Przykładowy skrypt dev build (macOS)

```bash
#!/usr/bin/env bash
# dev_install.sh — umieść w katalogu pluginu

PLUGIN_NAME="dmxnumeric"
BUILD_DIR="build"
BUNDLE="$HOME/QLC+.app"
DEST="$HOME/Library/Application Support/QLC+/VCWidgets"

# Build
cmake --build "$BUILD_DIR" || exit 1

DYLIB="$BUILD_DIR/lib${PLUGIN_NAME}_vcwidget.dylib"

# Naprawa rpath i codesign (wymagane po buildzie na macOS)
install_name_tool -add_rpath "$BUNDLE/Contents/Frameworks" "$DYLIB" 2>/dev/null || true
for fw in QtCore QtGui QtWidgets; do
  install_name_tool -change \
    "/opt/homebrew/opt/qtbase/lib/$fw.framework/Versions/A/$fw" \
    "/opt/homebrew/lib/$fw.framework/Versions/A/$fw" \
    "$DYLIB" 2>/dev/null || true
done
codesign --force --sign - "$DYLIB"

# Kopiuj — QLC+ wykryje zmianę i przeładuje w tle
cp "$DYLIB" "$DEST/"
echo "Installed $PLUGIN_NAME → QLC+ will hot-reload in ~1s"
```

### Folder "Watch extra folder" w Settings

Alternatywnie: w Settings podaj ścieżkę do katalogu build. QLC+ załaduje z niego pluginy i będzie je monitorować (plugin nie musi trafiać do folderu użytkownika).

---

## 16. Publikacja w globalnej bibliotece pluginów

### Jak działa biblioteka

QLC+ pobiera listę dostępnych pluginów z publicznego repozytorium GitHub:
`https://github.com/qlcplus/vcwidget-registry`

W dialogu **Get more widgets... → Browse Library** użytkownicy mogą przeglądać, instalować i aktualizować pluginy jednym kliknięciem.

### Jak zgłosić swój plugin

1. **Przygotuj binarki** dla każdej platformy (`.qlcvcw` z `manifest.json`)
2. **Utwórz release** na swoim GitHub z binarkami jako assets
3. **Otwórz Pull Request** do `qlcplus/vcwidget-registry` dodając entry do `index.json`:

```json
{
  "plugin_id": "org.twojadomena.vcwidgets.mojanazwa",
  "name": "Mój Widget",
  "version": "1.0.0",
  "author": "Imię Nazwisko",
  "category": "DMX Control",
  "description": "Krótki opis co robi widget.",
  "homepage": "https://github.com/ty/twoj-plugin",
  "min_qlc_version": "4.14.0",
  "platforms": {
    "macos_arm64": "https://github.com/ty/twoj-plugin/releases/download/v1.0.0/widget-1.0.0-macos-arm64.qlcvcw",
    "macos_x64":   "https://github.com/ty/twoj-plugin/releases/download/v1.0.0/widget-1.0.0-macos-x64.qlcvcw",
    "linux_x64":   "https://github.com/ty/twoj-plugin/releases/download/v1.0.0/widget-1.0.0-linux-x64.qlcvcw",
    "windows_x64": "https://github.com/ty/twoj-plugin/releases/download/v1.0.0/widget-1.0.0-windows-x64.qlcvcw"
  }
}
```

### GitHub Actions CI — automatyczna walidacja PR

Repozytorium `vcwidget-registry` zawiera GitHub Action który przy każdym PR:
- Waliduje JSON schema (`plugin_id`, `platforms` jako obiekt, wymagane pola)
- Sprawdza unikalność `plugin_id` w całym `index.json`
- Weryfikuje że URLe platform są osiągalne (HEAD request)

### Struktura repozytorium registry

```
qlcplus/vcwidget-registry/
├── index.json          ← lista wszystkich pluginów
├── schema.json         ← JSON Schema do walidacji
├── .github/
│   └── workflows/
│       └── validate.yml ← GitHub Action walidujący PR
└── README.md           ← instrukcja dla autorów
```

---

## 17. Cross-Project Copy/Paste

QLC+ obsługuje kopiowanie widgetów między projektami przez schowek systemowy (`Cmd+Shift+C` / `Cmd+Shift+V`). Mechanizm oparty jest na JSON — silnik automatycznie zapisuje `pluginId`, geometrię, caption, appearance i input sources. **Twój plugin dostaje cross-project copy/paste za darmo**, ale bez stanu specyficznego (np. przypisanych funkcji).

Żeby skopiować też stan specyficzny, nadpisz dwie wirtualne metody w klasie widgetu:

```cpp
// mójwidget.h
void toClipboardJson(QJsonObject &obj, const Doc *doc) const override;
void fromClipboardJson(const QJsonObject &obj, Doc *doc) override;
```

```cpp
// mójwidget.cpp
void MojWidget::toClipboardJson(QJsonObject &obj, const Doc *doc) const
{
    VCWidget::toClipboardJson(obj, doc);   // zapisze pluginId, geometry, caption, appearance, inputs

    // Zapisz stan specyficzny pluginu
    obj["threshold"]    = m_threshold;
    obj["functionName"] = m_function ? m_function->name() : QString();
    // Dla list funkcji:
    QJsonArray funcs;
    for (quint32 id : m_functionIds)
    {
        Function *f = doc->function(id);
        if (f) funcs.append(f->name());
    }
    if (!funcs.isEmpty())
        obj["functions"] = funcs;
}

void MojWidget::fromClipboardJson(const QJsonObject &obj, Doc *doc)
{
    VCWidget::fromClipboardJson(obj, doc); // odtworzy wspólną część

    // Odtwórz stan specyficzny
    m_threshold = obj["threshold"].toInt();

    // Funkcje — zawsze po nazwie (ID są projekt-specyficzne)
    if (Function *f = VCWidget::resolveFunctionByName(obj["functionName"].toString(), doc))
        setFunction(f->id());

    // Lista funkcji
    QList<quint32> ids;
    for (const QJsonValue &v : obj["functions"].toArray())
    {
        if (Function *f = VCWidget::resolveFunctionByName(v.toString(), doc))
            ids.append(f->id());
    }
    if (!ids.isEmpty())
        setFunctionIds(ids);

    update();
}
```

### Zasady serializacji

- **Funkcje zawsze po nazwie** — `Function::name()`, nigdy po `id()` (ID są projekt-specyficzne).
- **Fixture po nazwie** — `Fixture::name()` + numer kanału (nie adres DMX).
- Helper dla funkcji: `VCWidget::resolveFunctionByName(name, doc)` zwraca `Function*` lub `nullptr`.
- Fixture nie ma gotowego helpera — użyj bezpośrednio:

```cpp
// Serializacja (toClipboardJson):
Fixture *fxi = doc->fixture(m_fixtureId);
obj["fixtureName"] = fxi ? fxi->name() : QString();
obj["channel"]     = (int)m_channel;

// Deserializacja (fromClipboardJson):
const QString fxName = obj["fixtureName"].toString();
m_fixtureId = UINT_MAX;
for (Fixture *fxi : doc->fixtures())
{
    if (fxi && fxi->name() == fxName)
    {
        m_fixtureId = fxi->id();
        break;
    }
}
m_channel = (quint32)obj["channel"].toInt(0);
```

- Jeśli plugin nie nadpisze metod, QLC+ i tak skopiuje wygląd/rozmiar/caption/inputs — tylko stan specyficzny zostanie utracony przy wklejaniu do innego projektu.

### Rebuild

Metody `toClipboardJson`/`fromClipboardJson` są **wirtualne w `VCWidget`** i siedzą w Twojej `.dylib`. Po dodaniu/modyfikacji tych metod wystarczy przebudować **tylko plugin** (sekcja 12). QLC+ nie wymaga rebuildu.

### Co jeśli plugin nie jest zainstalowany

Jeśli próbujesz wkleić widget z pluginem który nie jest zainstalowany w projekcie docelowym, QLC+ wyświetli w oknie importu ostrzeżenie `[PLUGIN NOT INSTALLED: com.twoj.plugin]` i pominie ten widget.

---

## Checklist przed wydaniem pluginu

- [ ] `pluginId()` jest globalnie unikalny (reverse-domain)
- [ ] `writeDMX()` używa `GenericFader`, nie `Universe::write()`
- [ ] Wszystkie pola w `writeDMX()` chronione przez `QMutex`
- [ ] `slotModeChanged(Design)` woła `requestDelete()` na faderze
- [ ] Destruktor wyrejestrowuje `DMXSource` i kasuje fader
- [ ] `loadXML()` ma `root.skipCurrentElement()` dla nieznanych tagów
- [ ] `saveXML()` zapisuje `FixtureID` + `Channel`, nie surowe adresy
- [ ] `CMakeLists.txt` ustawia `.dylib` suffix na macOS
- [ ] Build z identycznym Qt prefix co QLC+ (`grep CMAKE_PREFIX_PATH CMakeCache.txt`)
- [ ] `install_name_tool` naprawa rpath + codesign po buildzie
- [ ] Jesli widget steruje funkcjami (nie DMX): `FunctionParent functionParent() const` zaimplementowane
- [ ] `f->stop(functionParent())` w destruktorze i `slotModeChanged(Design)` — brak stanu "wiszacych" funkcji
- [ ] Zadne pole/parametr/zmienna lokalna nie nazywa sie `slots` (konflikt z makro Qt)
- [ ] `manifest.json` używa `plugin_id` i `platforms` (snake_case, obiekt — nie tablica)
- [ ] Klucze w `platforms`: `macos_arm64`, `macos_x64`, `linux_x64`, `windows_x64`
- [ ] `manifest.json` ma poprawne nazwy bibliotek per platforma
- [ ] Widget definiuje `valueInputSourceId` i opcjonalnie `applyInputSourceId` jako `static const quint8`
- [ ] `slotInputValueChanged` override z `checkInputSource` dla każdego ID
- [ ] `updateFeedback()` override wysyła aktualną wartość przez `sendFeedback`
- [ ] Input source'y kopiowane w `createCopy`
- [ ] `<ValueInput>/<ApplyInput>` serializowane przez `saveXMLInput`/`loadXMLSources`
- [ ] `InputSelectionWidget` w dialogu konfiguracji dla każdego mapowania
- [ ] (opcjonalnie) `toClipboardJson` / `fromClipboardJson` nadpisane jeśli widget ma stan specyficzny (funkcje, fixture, parametry) — bez tego cross-project copy/paste kopiuje tylko wygląd
