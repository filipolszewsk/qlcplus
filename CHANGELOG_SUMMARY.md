# QLC+ Fork - Changelog (Podsumowanie)

## 🎯 Główne obszary rozwoju

---

## 1. 🌊 DimmerWave EFX
Nowy algorytm EFX inspirowany GrandMA3 z parametrami:
- Wave shape (sine, square, triangle, sawtooth)
- WaveLength - kontrola długości fali
- Propagacja fazy przez urządzenia
- Integracja z istniejącymi trybami EFX

---

## 2. 🎛️ Tryb Fixture Group dla EFX
Kompletna obsługa grup urządzeń w EFX:
- Automatyczne mapowanie urządzeń z siatki grupy
- Offsety fazy per kolumna z 6 wzorcami propagacji (LTR, RTL, Center-to-Sides, itp.)
- Tryb Wings (1-10 bloków) jak w GrandMA
- Filtr wyboru wierszy - wybierz które wiersze są aktywne
- Zachowanie ustawień Mode i Direction per kolumna
- Automatyczna przebudowa przy zmianach w grupie

---

## 3. 📊 RGB Matrix - Rozszerzenia
- **Multi-Value Matrix** - urządzenia mogą czytać z różnych wierszy skryptu
- **Per-Definition Channel Mapping** - mapowanie kanałów per typ urządzenia
- **Script Height** - dynamiczna wysokość matrycy w skryptach
- **Kontrolki Knob** - sterowanie parametrami skryptów przez pokrętła
- **Soft Patch** - miękkie patchowanie w VCMatrix
- **Row Filter** - filtrowanie wierszy

---

## 4. 📋 VCCueList - Nowe funkcje
- **Step Index Output** - wysyłanie numeru aktualnego kroku na kanał DMX
- **Tryb nagrywania** - nagrywanie scen bezpośrednio do cue listy
- **Funkcja Overwrite** - nadpisywanie istniejących kroków (także podczas aktywnego odtwarzania)
- **Przycisk Delete** - szybkie usuwanie kroków
- **Channel Columns** - kolumny wartości kanałów w widoku cue listy z edytorem kolumn, ukrywaniem i skalowanym trybem wyświetlania
- **Virtual Crossfade fix** - poprawione zachowanie crossfade z priorytetem LTP, reapply wartości kroków
- **Record/Overwrite/Delete external input** - sterowanie nagrywaniem, nadpisywaniem i usuwaniem z zewnętrznego kontrolera
- **Secondary selection via Next/Prev** - manualna kontrola wtórnego wyboru przez przyciski Next/Previous

---

## 5. 🎚️ VC Slider - Ulepszenia
- **Tryb One-Shot** - slider wysyła wartość tylko raz, pozostając w pozycji
- **Dual Input** - podwójne wejście zewnętrzne
- **Output Channel dla Multi-Patch** - przypisanie kanału wyjściowego

---

## 6. 🕹️ VCXYPad - Rozszerzenia
- **Tryb Fixture Group** - sterowanie grupami urządzeń
- **Soft Patch dla Presetów** - miękkie patchowanie presetów EFX/Scene
- **Multi-Selection** - wielokrotny wybór presetów
- **Naprawione crashe** - QPointer dla EFX/Scene, race conditions

---

## 7. 🔧 Fixture Group Editor - Ulepszenia
- **Multi-Selection** - zaznaczanie wielu urządzeń naraz
- **Drag & Drop z myszą** - przeciąganie urządzeń z podglądem na żywo
- **Klawisze strzałek** - przesuwanie zaznaczonych urządzeń
- **Logika zamiany** - poprawiona zamiana pozycji urządzeń

---

## 8. ↔️ Pan/Tilt Range
- Konfiguracja zakresów Pan/Tilt (np. 0-540°)
- Skalowanie w trybie relative EFX
- Reverse Pan/Tilt w modyfikatorach kanałów

---

## 9. 🔗 Crossfade & Cue Control
- Manualna kontrola wtórnego wyboru w trybie Crossfade
- Naprawione zachowanie dwukierunkowego slidera
- Poprawiony start crossfade i LTP priority przy reapply wartości

---

## 10. 🛠️ Multi-Patch Editor
- Opcja inkrementalna dla przypisań
- Poprawiona logika auto-increment
- Soft Patch dla Animation Knobs
- Naprawione przypisywanie wielu kanałów jednocześnie

---

## 11. 🐛 Krytyczne Bugfixy
- **DMX Dump overflow** - przepełnienie bufora dla wielu uniwersów
- **Autosave crash** - NULL check w Doc::saveXML
- **FixtureGroup race condition** - mutex locks
- **VCXYPad crash** - use-after-free naprawiony przez QPointer
- **RGBMatrix phase sync** - eliminacja błędów zaokrągleń przy zmianie prędkości
- **EFX zero range** - uruchamianie presetów z zerowym zakresem
- **VCSoloFrame page-aware stopping** - nie zatrzymuj funkcji na ukrytych/wyłączonych stronach
- **VCButton channel monitoring stability** - stabilizacja monitorowania kanałów z śledzeniem ownership funkcji

---

## 12. 🪟 VCFrame - Detach & Per-Page Size
- **Detach to Window** - odłączanie ramek do osobnych pływających okien z przyciskiem w headerze
- **Persistence** - zapis/odczyt stanu odłączonych okien z pliku projektu
- **Per-Page Size** - każda strona ramki może mieć inny rozmiar (checkbox w properties)
- **Reattachment fix** - poprawione ponowne dołączanie ramek do rodzica

---

## 13. 🔘 VCButton - Monitorowanie kanałów
- **Monitor Channel Values** - opcjonalne monitorowanie wartości kanałów DMX
- **Deactivate when overridden** - automatyczna dezaktywacja przycisku gdy kanał zostanie nadpisany
- **Function ownership tracking** - stabilne monitorowanie z śledzeniem właściciela funkcji

---

## 14. 📥 Import Functions Dialog
- **Import z innego projektu** - nowy dialog do importowania funkcji z plików .qxw
- **Fixture Mapping** - inteligentne mapowanie urządzeń (map to existing / create new / skip)
- **Auto-map by Name/Address** - automatyczne dopasowywanie urządzeń po nazwie lub adresie
- **Dependency resolution** - automatyczne rozwiązywanie zależności między funkcjami

---

## 15. 🗺️ Universe Grid View
- **Wizualizacja siatki universum** - graficzny widok kanałów z kolorami urządzeń
- **Drag & Drop** - zmiana adresu urządzenia przez przeciąganie w siatce
- **VC Input Remap** - automatyczne remapowanie wejść VC przy zmianie adresu urządzenia

---

## 16. 📋 RGBMatrix / EFX - Copy & Paste
- **Copy/Paste ustawień RGBMatrix** - kopiowanie i wklejanie parametrów matrycy z dialogiem wyboru
- **Batch Paste** - masowe wklejanie ustawień RGBMatrix do wielu funkcji jednocześnie (Ctrl+V w Function Manager)

---

## 17. 🏗️ Infrastruktura
- **Windows Build Workflow** - GitHub Actions dla instalatora Windows
- **Qt6 + MSYS2** - wsparcie dla nowoczesnego buildu
- **NSIS Installer** - tworzenie instalatorów .exe
- **SSH multi-account** - konfiguracja dwóch kont GitHub (fork/origin) przez SSH aliasy
- **macOS install helper** - skrypt pomocniczy instalacji na macOS

---

## 📈 Podsumowanie liczbowe

| Kategoria | Liczba |
|-----------|--------|
| Branchy lokalne | 64+ |
| Nowe funkcje | 50+ |
| Bugfixy | 12+ |
| Obszary rozwoju | 17 |

---

## 🎯 Kluczowe ulepszenia UX

1. **Workflow dla lightów ruchomych** - EFX z Fixture Groups, Wings, offsety
2. **Szybsza praca z Cue List** - nagrywanie, nadpisywanie, kolumny kanałów, crossfade fix
3. **Lepsza kontrola RGB Matrix** - pokrętła, multi-value, copy/paste ustawień
4. **Stabilność** - naprawione crashe, race conditions, SoloFrame i VCButton monitoring
5. **Fixture Group Editor** - drag & drop, multi-selection
6. **Import z projektów** - importowanie funkcji z innych plików .qxw z mapowaniem urządzeń
7. **Detached Frames** - odłączanie ramek VC do osobnych okien z zapisem stanu
8. **Universe Grid View** - wizualna siatka kanałów z drag & drop adresów

---

*Wersja: Fork QLC+ z rozszerzeniami dla zaawansowanych użytkowników*
*Data: 2026-02-20*
