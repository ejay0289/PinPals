#include <windows.h>
#include "Resource.h"

//control constants
#define ID_NEW_NOTE 2999
#define ID_TEXT 3001
#define ID_LABEL 3002
#define ID_PIN_BUTTON 3003

//Per note offsets for cbWndExtra
#define NOTE_HANDLE 0
#define NOTE_TOPMOST_STATE sizeof(LONG_PTR)

//custom messages
#define WM_APP_NOTE_CLOSED (WM_APP + 1)

//constants
char windowClass[] = "myWindowClass";
char myNoteClass[] = "myNoteclass";
char windowTitle[] = "PinPals";

//Globals
HWND hmainWindowHandle;
HWND* noteHandles = NULL;
int noteCount = 0;

LRESULT CALLBACK NoteWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{   
    int topMostState;
    



    switch (msg)
    {

    case WM_CREATE:
    {

        SetWindowLongPtr(hwnd, NOTE_TOPMOST_STATE, 0);
        HWND notePin = CreateWindowEx(
            0,"BUTTON","PIN",WS_CHILD | WS_VISIBLE|BS_PUSHBUTTON,
            0,0,50,50,hwnd,(HMENU)ID_PIN_BUTTON,GetModuleHandle(0),NULL
        );

        
        HWND textArea = CreateWindowEx(
            0, "EDIT", "Edit Me",
            WS_CHILD | WS_VISIBLE | ES_MULTILINE |ES_AUTOVSCROLL | WS_VSCROLL,
            0, 0, 100, 100, hwnd, (HMENU)ID_TEXT, GetModuleHandle(NULL),
            NULL
        );
        HFONT hfDefault = GetStockObject(DEFAULT_GUI_FONT);
        SendMessage(textArea, WM_SETFONT, (WPARAM)hfDefault, MAKELPARAM(FALSE, 0));
        SetWindowLongPtr(hwnd, 0, (LONG_PTR)textArea);
    }
        break;

    case WM_COMMAND:
    {
        int ctrlId = LOWORD(wParam);
        int notifCode = HIWORD(wParam);

        if (ctrlId == ID_PIN_BUTTON) {
            switch (notifCode) {
            case BN_CLICKED:
            {
                topMostState = (int)GetWindowLongPtr(hwnd, NOTE_TOPMOST_STATE);
                if (!topMostState) {
                    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                        SWP_NOMOVE | SWP_NOSIZE);
                    SetWindowLongPtr(hwnd, NOTE_TOPMOST_STATE, (LONG_PTR)1);
                }
                else { 
                    SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE); 
                    SetWindowLongPtr(hwnd, NOTE_TOPMOST_STATE, 0);

                }
            }
                break;
           }
        }
    }break;

    case WM_SIZE:
    {
        int width = LOWORD(lParam);
        int height = HIWORD(lParam);
        HWND textArea = (HWND)GetWindowLongPtr(hwnd, 0);
        MoveWindow(textArea, 50, 0, width-50, height, TRUE);
    }break;

    case WM_CLOSE:
        SendMessage(hmainWindowHandle, WM_APP_NOTE_CLOSED, 0, (LPARAM)hwnd);
        DestroyWindow(hwnd);
        break;
    case WM_DESTROY:
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {

    case WM_CREATE:
    {
        
        HWND labelNotes = CreateWindowEx(0,
            "STATIC", "Hello", WS_CHILD | WS_VISIBLE,
            10, 10, 200, 20, hwnd, (HMENU)ID_LABEL, GetModuleHandle(NULL),
            NULL);

        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)labelNotes);

        HWND button = CreateWindowEx(
            WS_EX_CLIENTEDGE, "BUTTON", "New Note",
            WS_CHILD | BS_PUSHBUTTON | WS_VISIBLE,
            100, 100, 100, 50, hwnd, (HMENU)ID_NEW_NOTE, GetModuleHandle(NULL),
            NULL
        );
 
    }break;

    case WM_APP_NOTE_CLOSED:
    {
        HWND closedChild = (HWND)lParam;

        for (int i = 0; i < noteCount;i++) {
            if (noteHandles[i] == closedChild) {
                noteHandles[i] = noteHandles[noteCount - 1];
                noteCount--;

                if (noteCount == 0) {
                    free(noteHandles);
                    noteHandles = NULL;
                }
                else {
                    HWND* pTemp = realloc(noteHandles, noteCount * sizeof(HWND));
                    if (pTemp == NULL)
                    {
                        MessageBox(hwnd, "Memory Allocation Failed", "Error!", MB_OK | MB_ICONERROR);
                        break;
                    }
                    else noteHandles = pTemp;
                }

                break;
            }

        }
    }break;


    case WM_COMMAND:
    {
        int ctrlId = LOWORD(wParam);
        int notifCode = HIWORD(wParam);

        if (ctrlId == ID_NEW_NOTE)
        {
            switch (notifCode)
            {
            case BN_CLICKED:
            {
                
                    HWND note = CreateWindowEx(
                        0,
                        myNoteClass,
                        windowTitle,
                        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                        CW_USEDEFAULT, CW_USEDEFAULT, 400, 400,
                        NULL, NULL, GetModuleHandle(NULL), NULL);

                    if (note == NULL) {
                        MessageBox(hwnd, "Note creation failed", "Error!", MB_OK | MB_ICONERROR);
                        break;
                    }

                    HWND* pTemp = realloc(noteHandles, (noteCount + 1) * sizeof(HWND));

                    if (pTemp == NULL) {
                        DestroyWindow(note);
                        MessageBox(hwnd, "Memory allocation for note failed", "Error!", MB_OK | MB_ICONERROR);
                        break;
                    }
                    else{
                        noteHandles = pTemp;
                        noteHandles[noteCount] = note;
                        noteCount++;

                    }

                    HWND labelNote = (HWND)GetWindowLongPtr(hwnd, GWLP_USERDATA);
                    char buffer[32];
                    wsprintf(buffer, "%d", noteCount);
                    SetWindowText(labelNote, buffer);
                

            }break;
            }
        }
    }break;

    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;
    case WM_DESTROY:
    {
        // Only quit if no notes are left
        for (int i = 0; i < noteCount; i++)
        {
            if (IsWindow(noteHandles[i]))
                return 0;  // leave message loop running
        }
        free(noteHandles);
        PostQuitMessage(0);
    }
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
    WNDCLASSEX wc;
    //HWND hwnd;
    MSG Msg;

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = 0;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = sizeof(LONG_PTR);
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PINPALS));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = windowClass;
    wc.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PINPALS));

    if (!RegisterClassEx(&wc))
    {
        MessageBox(NULL, "Window Registration Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    WNDCLASSEX noteClass = {0};
    noteClass.cbSize = sizeof(WNDCLASSEX);
    noteClass.style = 0;
    noteClass.lpfnWndProc = NoteWndProc;
    noteClass.cbClsExtra = 0;
    noteClass.cbWndExtra = 0;
    noteClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    noteClass.hInstance = hInstance;
    noteClass.lpszClassName = myNoteClass;
    noteClass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PINPALS));
    noteClass.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PINPALS));
    noteClass.cbWndExtra = sizeof(LONG_PTR) * 2;

    if (!RegisterClassEx(&noteClass))
    {
        MessageBox(NULL, "Note Registration Failed", "Error!", MB_ICONEXCLAMATION);
    }

    hmainWindowHandle = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        windowClass,
        windowTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 400,
        NULL, NULL, hInstance, NULL);

    if (hmainWindowHandle == NULL)
    {
        MessageBox(NULL, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    ShowWindow(hmainWindowHandle, nCmdShow);
    UpdateWindow(hmainWindowHandle);

    while (GetMessage(&Msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
    return Msg.wParam;
}