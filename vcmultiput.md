# VCMultiInputEditor

`VCMultiInputEditor` to edytor, który pozwala na przypisanie wielu zewnętrznych wejść (external inputs) do jednego widgetu w wirtualnej konsoli QLC+.

## Działanie

Gdy użytkownik zaznaczy kilka widgetów i otworzy edytor Multi-Input, `VCMultiInputEditor` zbiera wszystkie dostępne `inputSourceNames` z każdego zaznaczonego widgetu. Następnie tworzy listę unikalnych nazw wejść, które są wspólne dla wszystkich zaznaczonych widgetów.

Dla każdego wspólnego wejścia, edytor tworzy `InputSourceEditor`, który pozwala na skonfigurowanie zewnętrznego wejścia (universe, kanał, etc.).

## Liczba external inputs dla widgetów

Poniżej znajduje się lista widgetów i liczba ich zewnętrznych wejść, które powinny być obsługiwane przez `VCMultiInputEditor`:

*   **VCButton**: 1 external input
    *   Press/Release
*   **VCSlider**: 3 external inputs
    *   Value Control
    *   Reset to 0%
    *   Reset to 100%
*   **VCXYPad**: 6 external inputs
    *   X-Axis Control
    *   Y-Axis Control
    *   Reset X-Axis
    *   Reset Y-Axis
    *   Center X-Axis
    *   Center Y-Axis
*   **VCAudioTriggers**: 1 external input
    *   Trigger
*   **VCClock**: 2 external inputs
    *   Play/Pause
    *   Reset
*   **VCCueList**: 5 external inputs
    *   Next Cue
    *   Previous Cue
    *   Play/Stop
    *   Select Cue
    *   Playback
*   **VCFrame**: 2 external inputs
    *   Enable/Disable
    *   Next Page

## Problem

Obecnie `VCMultiInputEditor` niepoprawnie obsługuje widgety `VCCueList` i `VCFrame`, nie pokazując wszystkich dostępnych dla nich wejść. Należy to naprawić, aby wszystkie zdefiniowane `external_input` były widoczne i konfigurowalne w edytorze.

## Techniczne wpięcie do systemu External Input

Aby poprawnie zintegrować widget z systemem `external input` i `VCMultiInputEditor`, należy wykonać następujące kroki:

1.  **Zdefiniuj `inputSourceNames` w widgecie:** Każdy widget musi implementować metodę `inputSourceNames()`, która zwraca `QStringList` z nazwami wszystkich dostępnych wejść. Każda nazwa odpowiada jednemu parametrowi, który można kontrolować z zewnątrz.

    ```cpp
    // Przykład dla VCCueList
    QStringList VCCueList::inputSourceNames() const
    {
        QStringList list;
        list << tr("Next Cue");
        list << tr("Previous Cue");
        list << tr("Play/Stop");
        list << tr("Select Cue");
        list << tr("Playback");
        return list;
    }
    ```

2.  **Zmapuj `inputSource` na odpowiednie akcje:** W metodzie `slotInputValueChanged()` widgetu, należy obsłużyć przychodzące dane z `inputSource` i wywołać odpowiednią akcję. Identyfikacja `inputSource` odbywa się za pomocą `checkInputSource()`, która porównuje `universe` i `channel` przychodzącego sygnału z tymi zdefiniowanymi dla widgetu.

    ```cpp
    void VCCueList::slotInputValueChanged(quint32 universe, quint32 channel, uchar value)
    {
        if (checkInputSource(universe, channel, value, sender(), nextCueInputSourceId))
            slotNextCue();
        else if (checkInputSource(universe, channel, value, sender(), previousCueInputSourceId))
            slotPreviousCue();
        // ... i tak dalej dla pozostałych wejść
    }
    ```

3.  **Upewnij się, że `VCMultiInputEditor` poprawnie zbiera `inputSourceNames`:** Edytor powinien iterować po wszystkich zaznaczonych widgetach, pobierać ich `inputSourceNames` i tworzyć wspólną listę. Jeśli jakiś widget nie implementuje tej metody poprawnie, jego wejścia nie będą widoczne.

    Kluczowe jest, aby `VCMultiInputEditor` poprawnie identyfikował wszystkie widgety i ich możliwości. Problem w `VCCueList` i `VCFrame` wynikał z tego, że ich `inputSourceNames` nie były w pełni uwzględniane. Po poprawieniu implementacji w tych widgetach, edytor powinien działać zgodnie z oczekiwaniami.
