#include <windows.h>
#include <windowsx.h>
#include "Resource.h"
#include <string.h>

#define CON 0
//control constants
#define ID_NEW_NOTE 2999
#define ID_TEXT 3001
#define ID_LABEL 3002
#define ID_PIN_BUTTON 3003
#define ID_NEW_NOTE_BUTTON 3004
#define ID_SHOW_ALL_NOTES_BUTTON 3005
#define ID_CLOSE_ALL_BUTTON 3006

//Per note offsets for cbWndExtra
#define NOTE_HANDLE 0
#define NOTE_TOPMOST_STATE sizeof(LONG_PTR)
#define NEW_NOTE_BUTTON_HANDLE sizeof(LONG_PTR)

//custom messages
#define WM_APP_NOTE_CLOSED (WM_APP + 1)
#define WM_APP_NOTE_EDIT (WM_APP + 2)

//constants
char windowClass[] = "myWindowClass";
char myNoteClass[] = "myNoteclass";
char windowTitle[] = "PinPals";
#define MAX_NOTE_TEXT_LEN 1024
#define NOTE_MARGIN 10
#define NOTE_HEIGHT 100
#define NOTE_WIDTH 200
//Globals
HWND hmainWindowHandle;
HWND* noteHandles = NULL;
int noteCount = 0;
int scrollPos = 0;

struct NoteRectAndHandle {
    HWND hwnd;
    RECT rect;
    char text[MAX_NOTE_TEXT_LEN];
    int textLen;
};
struct NoteRectAndHandle* notes = NULL;


// Define the original procedure globally
WNDPROC g_OriginalEditProc = NULL;

LRESULT CALLBACK EditSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CHAR:
    {
        TCHAR charTyped = (TCHAR)wParam;

        if (charTyped > 31 || charTyped == '\b' || charTyped == '\n')
        {
            // **INTERCEPTED ACTION HERE
            char buffer[32];
            wsprintf(buffer, "%c", (int)charTyped);
            HWND hNoteParent = GetParent(hwnd);
            SendMessage(hmainWindowHandle, WM_APP_NOTE_EDIT, (WPARAM)charTyped, (LPARAM)hNoteParent);
            
            return CallWindowProc(g_OriginalEditProc,hwnd,msg,wParam,lParam); //Message will be passed to parent
        }
    }
    break;
    }
    return CallWindowProc(g_OriginalEditProc, hwnd, msg, wParam, lParam);
}


static void RecalculateNotePositions(HWND hwnd) {
    int x = NOTE_MARGIN;
    int y = NOTE_MARGIN;
    int totalContentHeight = 0;

    for (int i = 0; i < noteCount; i++)
    {
        notes[i].rect.left = x;
        notes[i].rect.top = y;

        notes[i].rect.right = x + NOTE_WIDTH;
        notes[i].rect.bottom = y + NOTE_HEIGHT;

        y += NOTE_HEIGHT + NOTE_MARGIN; // stack vertically
    }
    
    RECT client;
    GetClientRect(hwnd, &client);
    SendMessage(hwnd, WM_SIZE, 0, MAKELPARAM(client.right - client.left, client.bottom - client.top));
}

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


        HWND showAllNotes = CreateWindowEx(
            0, "BUTTON", "Show all", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            0, 100, 50, 50, hwnd, (HMENU)ID_SHOW_ALL_NOTES_BUTTON, GetModuleHandle(0), NULL
        );

        HWND newNoteButton = CreateWindowEx(
            0, "BUTTON", "New", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            0, 50, 50, 50, hwnd, (HMENU)ID_NEW_NOTE_BUTTON, GetModuleHandle(0), NULL
        );

        HWND textArea = CreateWindowEx(
            0, "EDIT", "",
            WS_CHILD | WS_VISIBLE | ES_MULTILINE |ES_AUTOVSCROLL | WS_VSCROLL,
            0, 0, 100, 100, hwnd, (HMENU)ID_TEXT, GetModuleHandle(NULL),
            NULL
        );
        HFONT hfDefault = GetStockObject(DEFAULT_GUI_FONT);
        SendMessage(textArea, WM_SETFONT, (WPARAM)hfDefault, MAKELPARAM(FALSE, 0));
        SetWindowLongPtr(hwnd, 0, (LONG_PTR)textArea);

        g_OriginalEditProc = (WNDPROC)SetWindowLongPtr(
            textArea,                  
            GWLP_WNDPROC,               
            (LONG_PTR)EditSubclassProc  
        );

    }
        break;

    case WM_COMMAND:
    {
        int ctrlId = LOWORD(wParam);
        int notifCode = HIWORD(wParam);

            switch (notifCode) {
            case BN_CLICKED:
                if (ctrlId == ID_PIN_BUTTON) {
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
                }
                else if (ctrlId == ID_SHOW_ALL_NOTES_BUTTON) {
                    ShowWindow(hmainWindowHandle,SW_SHOW);
                    
                }

                else if (ctrlId == ID_NEW_NOTE_BUTTON) {
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
                    else {
                        noteHandles = pTemp;
                        noteHandles[noteCount] = note;

                        notes = realloc(notes, sizeof(struct NoteRectAndHandle) * (noteCount + 1));
                        if (notes == NULL) {
                            MessageBox(hwnd, "Memory allocation failed", "!Error", MB_OKCANCEL);
                            break;
                        }
                        else {
                            notes[noteCount] = (struct NoteRectAndHandle){ .hwnd = note, .rect = { 0,0,0,0 } };
                           
                            noteCount++;
                            RecalculateNotePositions(hwnd);
                            InvalidateRect(hwnd, NULL, TRUE);
                            UpdateWindow(hwnd);
                        }
                    }
                }
                break;
        }
    }break;

    case WM_SIZE:
    {
        int width = LOWORD(lParam);
        int height = HIWORD(lParam);
        HWND textArea = (HWND)GetWindowLongPtr(hwnd, 0);
        HWND pinButton = GetDlgItem(hwnd, ID_PIN_BUTTON);
        HWND showAllButton = GetDlgItem(hwnd, ID_SHOW_ALL_NOTES_BUTTON);
        HWND newNoteButton = GetDlgItem(hwnd, ID_NEW_NOTE_BUTTON);

        int buttonWidth = 50;
        int buttonHeight = 50;


        MoveWindow(pinButton, 0, 0, buttonWidth, buttonHeight, TRUE);
        MoveWindow(newNoteButton, 0, buttonHeight, buttonWidth, buttonHeight, TRUE);
        MoveWindow(showAllButton, 0, buttonHeight * 2, buttonWidth, buttonHeight, TRUE);

//Fit text area on resize
       MoveWindow(textArea, buttonWidth, 0, width - buttonWidth, height, TRUE);
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
		
		
	case WM_LBUTTONDOWN:
	{
		POINT ptClick;
		ptClick.x = GET_X_LPARAM(lParam);
		ptClick.y = GET_Y_LPARAM(lParam);
		
		char buffer[64];
		
		for(int i = 0; i < noteCount; i++)
		{
			if(PtInRect(&notes[i].rect,ptClick))
			{
				//sprintf(buffer, sizeof(buffer), "Clicked Note Handle: %p", notes[i].hwnd);
				MessageBox(hwnd,notes[i].text,"Clicked",MB_OK);
				break;
			}
		}
	}break;

    case WM_APP_NOTE_EDIT:
    {
        TCHAR charTyped = (TCHAR)wParam;
        HWND hNoteParent = (HWND)lParam;

        // 1. Find the corresponding NoteRectAndHandle structure
        for (int i = 0; i < noteCount; i++)
        {
            if (notes[i].hwnd == hNoteParent)
            {
                // 2. Update the text buffer based on the character
                int currentLen = notes[i].textLen;

                if (charTyped == '\b')
                {
                    if (currentLen > 0)
                    {
                        notes[i].text[currentLen - 1] = '\0'; // Null-terminate the new end
                        notes[i].textLen = currentLen - 1;
                    }
                }
                else 
                {
                    if (currentLen < MAX_NOTE_TEXT_LEN - 1)
                    {
                        notes[i].text[currentLen] = charTyped;
                        notes[i].text[currentLen + 1] = '\0'; // Null-terminate
                        notes[i].textLen = currentLen + 1;
                    }
                }

                // 3. Force the main window to redraw (where the RECTs are drawn)
                InvalidateRect(hwnd, &notes[i].rect, TRUE);
                UpdateWindow(hwnd);

                break; // Found and processed the note, exit loop
            }
        }
    }
    return 0;

    case WM_SIZE:
    {
        RECT rect;
        GetClientRect(hwnd, &rect);
       int windowWidth = rect.right - rect.left;
       int windowHeight = rect.bottom - rect.top;
       int scrollbarWidth = GetSystemMetrics(SM_CXVSCROLL);
       
       int totalContentHeight = (noteCount > 0)
           ? notes[noteCount - 1].rect.bottom + NOTE_MARGIN // Bottom of last note + final margin
           : 0;

       SCROLLINFO si = { 0 };
       si.cbSize = sizeof(si);
       si.fMask = SIF_RANGE | SIF_PAGE;
       si.nMin = 0;
       //THIS SHIT FAILED WHY IS SCROLLING SO HARD FUCKKKK

       // Set max range to the bottom of the content.
       // The range represents the *top* of the window when fully scrolled down.
       // The max position is totalContentHeight - windowHeight. 
       // The 'nMax' parameter here must be greater than 'nMin' and includes nPage.
       // A more common approach is setting nMax to the full content height.
       si.nMax = totalContentHeight;
       si.nPage = windowHeight;
       SetScrollInfo(hwnd, SB_VERT, &si, TRUE);

       int yScrollPos = GetScrollPos(hwnd, SB_VERT);
       int maxScroll = max(0, totalContentHeight - windowHeight);

       if (yScrollPos > maxScroll) {
           ScrollWindowEx(hwnd, 0, yScrollPos - maxScroll, NULL, NULL, NULL, NULL, SW_INVALIDATE);
           SetScrollPos(hwnd, SB_VERT, maxScroll, TRUE);
           UpdateWindow(hwnd);
       }

       int topRightX = windowWidth - scrollbarWidth - 100; // top-right 
       int topRightY = 0;

       HWND newNoteButton = (HWND)GetWindowLongPtr(hwnd, NEW_NOTE_BUTTON_HANDLE);
        MoveWindow(newNoteButton, topRightX, topRightY, 100, 50, TRUE);
        HWND closeAllButton = GetDlgItem(hwnd, ID_CLOSE_ALL_BUTTON);
        MoveWindow(closeAllButton, topRightX, topRightY + 50, 100, 50, TRUE);

    }break;

    case WM_CREATE:
    {
        RECT rec;
        GetClientRect(hwnd, &rec);
        int windowWidth = rec.right - rec.left;
        int scrollbarWidth = GetSystemMetrics(SM_CXVSCROLL);
        int topRightX = windowWidth - scrollbarWidth - 100;


        HWND newNoteButton = CreateWindowEx(
            0, "BUTTON", "New Note",
            WS_CHILD | BS_PUSHBUTTON | WS_VISIBLE,
            topRightX, 50, 100, 50, hwnd, (HMENU)ID_NEW_NOTE, GetModuleHandle(NULL),
            NULL
        );

        HWND closeAllButton = CreateWindowEx(0, "BUTTON", "Close",
            WS_CHILD | BS_PUSHBUTTON | WS_VISIBLE,
            topRightX, 0, 100, 50, hwnd, (HMENU)ID_CLOSE_ALL_BUTTON, GetModuleHandle(0), NULL
        );

        SetWindowLongPtr(hwnd, NEW_NOTE_BUTTON_HANDLE, (LONG_PTR)newNoteButton);
        SendMessage(hwnd, WM_SIZE, 0, MAKELPARAM(windowWidth, rec.bottom - rec.top));
////////////////////////////////////////////////////
      
       
 
    }break;

    case WM_VSCROLL:
    {
        SCROLLINFO si = { 0 };
        si.cbSize = sizeof(si);
        si.fMask = SIF_ALL;
        GetScrollInfo(hwnd, SB_VERT, &si);

        int yOldPos = si.nPos;
        int yNewPos = yOldPos;

        RECT rect;
        GetClientRect(hwnd, &rect);
        int windowHeight = rect.bottom - rect.top;

        int totalContentHeight = (noteCount > 0)
            ? notes[noteCount - 1].rect.bottom + NOTE_MARGIN
            : 0;
        int maxScroll = max(0, totalContentHeight - windowHeight);

        switch (LOWORD(wParam))
        {
        case SB_LINEUP: yNewPos -= NOTE_HEIGHT / 2; break;
        case SB_LINEDOWN: yNewPos += NOTE_HEIGHT / 2; break;
        case SB_PAGEUP: yNewPos -= windowHeight; break;
        case SB_PAGEDOWN: yNewPos += windowHeight; break;
        case SB_THUMBTRACK: yNewPos = HIWORD(wParam); break;
        default: return 0;
        }

        yNewPos = max(0, yNewPos);
        yNewPos = min(yNewPos, maxScroll);

        if (yNewPos == yOldPos) return 0;

        int scrollAmount = yOldPos - yNewPos; // Positive for scrolling content UP

        SetScrollPos(hwnd, SB_VERT, yNewPos, TRUE);

        RECT rcUpdate;

        ScrollWindowEx(hwnd,
            0,
            scrollAmount,
            NULL,       
            NULL,       
            &rcUpdate,  
            NULL,       
            0           
        );


        InvalidateRect(hwnd, &rcUpdate, TRUE);
        UpdateWindow(hwnd);
    }
    return 0;


    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        int yScrollPos = GetScrollPos(hwnd, SB_VERT);
       SetViewportOrgEx(hdc, 0, -yScrollPos, NULL);

        // Set brush color for notes
        HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 0)); // yellow notes
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, hBrush);
        SetBkMode(hdc, TRANSPARENT);

        // Draw all notes
        for (int i = 0; i < noteCount; i++)
        {
            Rectangle(hdc, notes[i].rect.left, notes[i].rect.top, notes[i].rect.right, notes[i].rect.bottom);
            if (notes[i].textLen > 0)
            {
                // Use the note's rect for the drawing area
                RECT textRect = notes[i].rect;

                // Adjust the rect inward slightly for padding
                InflateRect(&textRect, -2, -2);

                DrawText(hdc,
                    notes[i].text,
                    notes[i].textLen,
                    &textRect,
                    DT_LEFT | DT_TOP | DT_WORDBREAK);
            }
        }


       
        // Cleanup
        SelectObject(hdc, oldBrush);
        DeleteObject(hBrush);
        EndPaint(hwnd, &ps);
        return 0;
    }
    break;

    case WM_APP_NOTE_CLOSED:
    {
        HWND closedChild = (HWND)lParam;

         for (int i = 0; i < noteCount;i++) {
            if (noteHandles[i] == closedChild && !(notes[i].textLen > 0)) {
                noteHandles[i] = noteHandles[noteCount - 1];
                    notes[i] = notes[noteCount - 1];             
                    noteCount--;

                if (noteCount > 0) {
                    struct NoteRectAndHandle* pTempNotes = realloc(notes, noteCount * sizeof(struct NoteRectAndHandle));
                    if (pTempNotes != NULL) notes = pTempNotes;
                    HWND* pTemp = realloc(noteHandles, noteCount * sizeof(HWND));
                    if (pTemp != NULL) noteHandles = pTemp;
                }
                else {
                    free(notes);
                    notes = NULL;
                    free(noteHandles);
                    noteHandles = NULL;
                }
                RecalculateNotePositions(hwnd);

                InvalidateRect(hwnd, NULL, TRUE);
                break;
            }

        }
    }break;


    case WM_COMMAND:
    {
        int ctrlId = LOWORD(wParam);
        int notifCode = HIWORD(wParam);

            switch (notifCode)
            {
            case BN_CLICKED:
            {
                
        if (ctrlId == ID_NEW_NOTE){
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
                        
                        notes = realloc(notes, sizeof(struct NoteRectAndHandle) * (noteCount + 1));
                        if (notes == NULL) {
                            MessageBox(hwnd, "Memory allocation failed", "!Error", MB_OKCANCEL);
                            break;
                        }
                        else {
                            notes[noteCount] = (struct NoteRectAndHandle){ .hwnd = note, .rect = { 0,0,0,0 } };
                            notes[noteCount].text[0] = '\0';
                            notes[noteCount].textLen = 0;

                            int x = NOTE_MARGIN;
                            int y = (NOTE_HEIGHT + 2 * NOTE_MARGIN) * noteCount;
                            
                            //UNCOMMENT THIS IF BROKEN

                               // notes[noteCount].rect.left = x;
                               // notes[noteCount].rect.top = y;
                               // notes[noteCount].rect.right = x + NOTE_HEIGHT;
                                //notes[noteCount].rect.bottom = y + NOTE_HEIGHT;

                                //y += NOTE_SIZE + NOTE_MARGIN; // stack vertically
                            
                            noteCount++;
                            RecalculateNotePositions(hwnd);

                            InvalidateRect(hwnd, NULL, TRUE);
                            SendMessage(hwnd, WM_SIZE, 0, 0);
                            InvalidateRect(hwnd, NULL, TRUE);
                            UpdateWindow(hwnd);
                        }
                    }
                    

        }
        else if (ctrlId == ID_CLOSE_ALL_BUTTON) {
            free(noteHandles);
            free(notes);
            PostQuitMessage(0);
        }break;
            }
        }
    }break;

    case WM_CLOSE:
        //DestroyWindow(hwnd);
        ShowWindow(hwnd, SW_HIDE);
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
    MSG Msg;

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = 0;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = sizeof(LONG_PTR) *2;
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
        WS_OVERLAPPEDWINDOW | WS_VSCROLL ,
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