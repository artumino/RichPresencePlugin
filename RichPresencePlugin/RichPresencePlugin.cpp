// RichPresenceTest.cpp : Questo file contiene la funzione 'main', in cui inizia e termina l'esecuzione del programma.
//

#include <iostream>
#include "RichPresenceComponent.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    UINT parentId = NULL;
    RichPresenceComponent* test;

    const wchar_t* nameCommandLinePart = L"-name:";
    wchar_t* commandLineMatch = wcsstr(GetCommandLine(), nameCommandLinePart);
    std::wstring gameName;
    if (commandLineMatch)
        gameName = std::wstring(&commandLineMatch[wcslen(nameCommandLinePart)]);
    else
        gameName = L"Test App";


    const wchar_t* parentCommandLinePart = L"-pid:";
    commandLineMatch = wcsstr(GetCommandLine(), parentCommandLinePart);
    if(commandLineMatch)
        parentId = _wtoi(&commandLineMatch[wcslen(parentCommandLinePart)]);

    const wchar_t* childCommandLinePart = L"-child:";
    commandLineMatch = wcsstr(GetCommandLine(), childCommandLinePart);
    if (commandLineMatch)
    {
        parentId = _wtoi(&commandLineMatch[wcslen(childCommandLinePart)]);
        test = new RichPresenceComponent(parentId, gameName.c_str());
        test->InitializeGameChild();
    }
    else
    {
        childCommandLinePart = L"-parent:";
        commandLineMatch = wcsstr(GetCommandLine(), childCommandLinePart);
        if (commandLineMatch)
        {
            test = new RichPresenceComponent(parentId, gameName.c_str());
            test->InitializeGameParent();
        }
        else
        {
            test = new RichPresenceComponent(parentId, gameName.c_str());
            test->InitializeMain();
        }
    }

    delete test;
}

// Per eseguire il programma: CTRL+F5 oppure Debug > Avvia senza eseguire debug
// Per eseguire il debug del programma: F5 oppure Debug > Avvia debug

// Suggerimenti per iniziare: 
//   1. Usare la finestra Esplora soluzioni per aggiungere/gestire i file
//   2. Usare la finestra Team Explorer per connettersi al controllo del codice sorgente
//   3. Usare la finestra di output per visualizzare l'output di compilazione e altri messaggi
//   4. Usare la finestra Elenco errori per visualizzare gli errori
//   5. Passare a Progetto > Aggiungi nuovo elemento per creare nuovi file di codice oppure a Progetto > Aggiungi elemento esistente per aggiungere file di codice esistenti al progetto
//   6. Per aprire di nuovo questo progetto in futuro, passare a File > Apri > Progetto e selezionare il file con estensione sln
