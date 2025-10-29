#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <windowsx.h>
#include "Resource.h"
#include <string.h>
#include "sqlite3.h"
#include <stdio.h>

#include "PinPals.h"

#define CON 0
//control constants
#define ID_NEW_NOTE 2999
#define ID_TEXT 3001
#define ID_LABEL 3002
#define ID_PIN_BUTTON 3003
#define ID_NEW_NOTE_BUTTON 3004
#define ID_SHOW_ALL_NOTES_BUTTON 3005
#define ID_CLOSE_ALL_BUTTON 3006
#define ID_SAVE_NOTE_BUTTON 3007

//Per note offsets for cbWndExtra
#define NOTE_HANDLE 0
#define NOTE_TOPMOST_STATE sizeof(LONG_PTR)
#define NEW_NOTE_BUTTON_HANDLE sizeof(LONG_PTR)
//custom messages
#define WM_APP_NOTE_CLOSED (WM_APP + 1)
#define WM_APP_NOTE_DELETED (WM_APP + 2)
#define WM_APP_NOTE_EDIT (WM_APP + 3)
#define WM_APP_SAVE (WM_APP + 4)
#define WM_APP_CALL_UPDATE_WINDOW (WM_APP + 5)

//constants
char windowClass[] = "myWindowClass";
char myNoteClass[] = "myNoteclass";
char windowTitle[] = "PinPals";
#define NOTE_MARGIN 10
#define NOTE_HEIGHT 100
#define NOTE_WIDTH 200
//Globals
sqlite3 *db = NULL;
HWND hmainWindowHandle;
int noteCount = 0;
int scrollPos = 0;
struct Note* notes_true = NULL;
struct Note* notes_unsaved = NULL;



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
            
            return CallWindowProc(g_OriginalEditProc,hwnd,msg,wParam,lParam);
        }
    }
    break;
    }
    return CallWindowProc(g_OriginalEditProc, hwnd, msg, wParam, lParam);
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


        HWND newNoteButton = CreateWindowEx(
            0, "BUTTON", "New", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            0, 50, 50, 50, hwnd, (HMENU)ID_NEW_NOTE_BUTTON, GetModuleHandle(0), NULL
        );
        HWND showAllNotes = CreateWindowEx(
            0, "BUTTON", "Show all", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            0, 100, 50, 50, hwnd, (HMENU)ID_SHOW_ALL_NOTES_BUTTON, GetModuleHandle(0), NULL
        );

        HWND saveButton = CreateWindowEx(
            0, "BUTTON", "Save", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            0, 150, 50, 50, hwnd, (HMENU)ID_SAVE_NOTE_BUTTON, GetModuleHandle(0), NULL
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
					WORD ctrlId = ID_NEW_NOTE;
					WORD notifCode = BN_CLICKED;
					
					PostMessage(hmainWindowHandle,WM_COMMAND,MAKELPARAM(ctrlId,notifCode),0);
					PostMessage(hmainWindowHandle,WM_PAINT,(WPARAM)hwnd,0);
                }

                else if(ctrlId == ID_SAVE_NOTE_BUTTON){
                    SendMessage(hmainWindowHandle, WM_APP_SAVE, (WPARAM)hwnd, (LPARAM)hwnd);
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
        SendMessage(hmainWindowHandle, WM_APP_NOTE_CLOSED, (WPARAM)hwnd, (LPARAM)hwnd);
		//If I destroy I will have to spawn a new window, grab the text(from the
		//RECT for now until I can get SQLite running) stick it in the edit and somehow reestablish abort
		//link between the new window and the note
        //DestroyWindow(hwnd);
		ShowWindow(hwnd,SW_HIDE);
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
    //For tracking note updates
    static char* idIsPresent = NULL;
    static int noteUpdateId = 0;


    switch (msg)
    {
	case WM_LBUTTONDOWN:
	{
		POINT ptClick;
		ptClick.x = GET_X_LPARAM(lParam);
		ptClick.y = GET_Y_LPARAM(lParam);
		//Note deletion
		for(int i = 0; i < noteCount; i++)
		{
			RECT rectXButton;
            int buttonSize = 15;
            rectXButton.left   = notes_true[i].rect.right - buttonSize;
            rectXButton.top    = notes_true[i].rect.top;
            rectXButton.right  = notes_true[i].rect.right;
            rectXButton.bottom = notes_true[i].rect.top + buttonSize;
			
            if (PtInRect(&rectXButton, ptClick))
            {
                SendMessage(hwnd, WM_APP_NOTE_DELETED, (WPARAM)notes_true[i].id, (LPARAM)notes_true[i].id);
                break;
            }
            
			if(PtInRect(&notes_true[i].rect,ptClick) && PtInRect(&notes_true[i].rect, ptClick))
			{
                int noteId = notes_true[i].id;
                int noteLength = notes_true[i].textLen;
                SendMessage(hwnd, WM_APP_CALL_UPDATE_WINDOW, (WPARAM)noteId, (LPARAM)noteLength);
			break;
			}
		}
	}break;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    //TODO: Make real time updates appear in note RECT with WP_APP_NOTE_EDIT: Worked in a previous implepmentation but broke
    case WM_APP_NOTE_EDIT:
    {
        TCHAR charTyped = (TCHAR)wParam;
        HWND hNoteParent = (HWND)lParam;

        for (int i = 0; i < noteCount; i++)
        {
            if (notes_true[i].text == hNoteParent)
            {
                // Update the text buffer based on the character
                int currentLen = notes_true[i].textLen;

                if (charTyped == '\b')
                {
                    if (currentLen > 0)
                    {
                        notes_true[i].text[currentLen - 1] = '\0'; 
                        notes_true[i].textLen = currentLen - 1;
                    }
                }
                else 
                {
                    if (currentLen < MAX_NOTE_TEXT_LEN - 1)
                    {
                        notes_true[i].text[currentLen] = charTyped;
                        notes_true[i].text[currentLen + 1] = '\0'; 
                        notes_true[i].textLen = currentLen + 1;
                    }
                }

                InvalidateRect(hwnd, &notes_true[i].rect, TRUE);
                UpdateWindow(hwnd);
                break; 
            }
        }
    }
    return 0;

    //Wndproc
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    case WM_SIZE:
    {
        RECT rect;
        GetClientRect(hwnd, &rect);
       int windowWidth = rect.right - rect.left;
       int windowHeight = rect.bottom - rect.top;
       int scrollbarWidth = GetSystemMetrics(SM_CXVSCROLL);
       
       int totalContentHeight = (noteCount > 8)
           ? notes_true[noteCount - 1].rect.bottom + NOTE_MARGIN 
           : 0;

       SCROLLINFO si = { 0 };
       si.cbSize = sizeof(si);
       si.fMask = SIF_RANGE | SIF_PAGE;
       si.nMin = 0;
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
       int topRightY = 0;                                  //

       HWND newNoteButton = (HWND)GetWindowLongPtr(hwnd, NEW_NOTE_BUTTON_HANDLE);
        MoveWindow(newNoteButton, topRightX, topRightY, 100, 50, TRUE);
        HWND closeAllButton = GetDlgItem(hwnd, ID_CLOSE_ALL_BUTTON);
        MoveWindow(closeAllButton, topRightX, topRightY + 50, 100, 50, TRUE);

    }break;
    //WndProc
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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
 
    }break;

    /////////////////////////////////////////////////////////////////////////////////////////////////////      


    //TODO: Implement scrolling
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
            ? notes_true[noteCount - 1].rect.bottom + NOTE_MARGIN
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

        int scrollAmount = yOldPos - yNewPos; 
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
    //WndProc
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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

        int theY = NOTE_MARGIN;

        for (int i = 0; i < noteCount; i++)
        {
            notes_true[i].rect.left = NOTE_MARGIN;
            notes_true[i].rect.top = theY;
            notes_true[i].rect.right = NOTE_MARGIN + NOTE_WIDTH;
            notes_true[i].rect.bottom = theY + NOTE_HEIGHT;
            theY += NOTE_HEIGHT + NOTE_MARGIN; // stack vertically
        }

    //Draw notes to main window
        for (int i = 0; i < noteCount; i++)
        {
            Rectangle(hdc, notes_true[i].rect.left, notes_true[i].rect.top, notes_true[i].rect.right, notes_true[i].rect.bottom);
            RECT textRect = notes_true[i].rect;
            InflateRect(&textRect, -5, -5);

            // fetch content(text) from DB using each note's unique ID
            char* content = getDatabaseEntry(notes_true[i].id);
            if (content) {
                DrawTextA(
                    hdc,
                    content,
                    -1,
                    &textRect,
                    DT_LEFT | DT_TOP | DT_WORDBREAK
                );
                free(content);
            }
            else {
                DrawTextA(
                    hdc,
                    "(empty)",
                    -1,
                    &textRect,
                    DT_LEFT | DT_TOP | DT_WORDBREAK
                );
            }
  
			RECT rectXButton;
            int buttonSize = 15;
            rectXButton.left   = notes_true[i].rect.right - buttonSize;
            rectXButton.top    = notes_true[i].rect.top;
            rectXButton.right  = notes_true[i].rect.right;
            rectXButton.bottom = notes_true[i].rect.top + buttonSize;
            DrawText(hdc, "X", 1, &rectXButton, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        }

        // Cleanup
        SelectObject(hdc, oldBrush);
        DeleteObject(hBrush);
        EndPaint(hwnd, &ps);
        return 0;
    }
    break;
    

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    case WM_APP_CALL_UPDATE_WINDOW:
    {

        int noteId = (int)wParam;
        char* noteValue = getDatabaseEntry(noteId);
        HWND editNote = CreateWindowEx(
            0,
            myNoteClass,
            windowTitle,
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT, CW_USEDEFAULT, 400, 400,
            NULL, NULL, GetModuleHandle(NULL), NULL);
        int hEdit = GetDlgItem(editNote, ID_TEXT);
        if (hEdit && noteValue) {
            SetWindowText(hEdit, noteValue);
}
                
        idIsPresent = getDatabaseEntry(noteId);
        noteUpdateId = noteId;
        free(noteValue);
        noteValue = NULL;

    }break;
	
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	case WM_APP_NOTE_DELETED:
    {
        int noteId = (int)lParam;

        for (int i = 0; i < noteCount; i++) {
            if (notes_true[i].id == noteId) {

                deleteNoteFromDatabase(noteId);
                for (int j = i; j < noteCount - 1; j++) {
                    notes_true[j] = notes_true[j + 1];
                }
                noteCount--;

                if (noteCount > 0) {
                    struct Note* pTempNotes = realloc(notes_true, noteCount * sizeof(struct Note));
                    if (pTempNotes != NULL) notes_true = pTempNotes;
                    
                }
                else {
                    free(notes_true);
                    notes_true = NULL;

                }
                RecalculateNotePositions(hwnd);
                InvalidateRect(hwnd, NULL, TRUE);
                break;
            }
        }
    }
    break;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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
                        
                        notes_true = realloc(notes_true, sizeof(struct Note) * (noteCount + 1));
                        if (notes_true == NULL) {
                            MessageBox(hwnd, "Memory allocation failed", "!Error", MB_OKCANCEL);
                            break;
                        }
                        else {
							//TODO:Shift all notes up an index and place new note at index 0;
			
                            notes_true[noteCount] = (struct Note){0};                  
                            noteCount++;
							SendMessage(hwnd, WM_SIZE, 0, 0);
							
                            RecalculateNotePositions(hwnd);
                            InvalidateRect(hwnd, NULL, TRUE);
                            UpdateWindow(hwnd);
                        }
        }
        else if (ctrlId == ID_CLOSE_ALL_BUTTON) {
            free(notes_true);
            sqlite3_close(db);
            PostQuitMessage(0);
        }
        
        else if (ctrlId == ID_SAVE_NOTE_BUTTON) {
            SendMessage(hmainWindowHandle,WM_APP_SAVE, (WPARAM)hwnd, (LPARAM)hwnd);
        }break;
            }
        }
    }break;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    case WM_CLOSE:
        //DestroyWindow(hwnd);
        ShowWindow(hwnd, SW_HIDE);
        break;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    case WM_APP_SAVE:
    {
        LPARAM noteHandle = (LPARAM)lParam;
        HWND hEdit = GetDlgItem(noteHandle, ID_TEXT);
        int length = GetWindowTextLength(hEdit);
        if (length <= 0) break;
        char* buffer = malloc(length + 1);
        GetWindowText(hEdit, buffer, length + 1);
        char sql[512];

        if (idIsPresent) {
            updateDatabaseEntry(noteUpdateId, buffer);
        }
        else {
            snprintf(sql, sizeof(sql),
                "INSERT INTO notes (title, content) VALUES ('New Note', '%s');", buffer);

            char* errmsg = NULL;
            int rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);

            if (rc != SQLITE_OK) {
                MessageBoxA(hwnd, errmsg ? errmsg : "Database insert failed", "SQLite Error", MB_OK | MB_ICONERROR);
                sqlite3_free(errmsg);
            }
            else {
                sqlite3_int64 lastId = sqlite3_last_insert_rowid(db);
                notes_true[noteCount - 1].id = (int)lastId;
                strncpy_s(notes_true[noteCount - 1].title, sizeof(notes_true[noteCount - 1].title), "New Note", _TRUNCATE);
                strncpy_s(notes_true[noteCount - 1].text, sizeof(notes_true[noteCount - 1].text), buffer, _TRUNCATE);
                notes_true[noteCount - 1].textLen = length;
                notes_true[noteCount - 1].rect = (RECT){ 0,0,0,0 };
            }
        }
        RecalculateNotePositions(hwnd);
        InvalidateRect(hwnd, NULL, TRUE);
        UpdateWindow(hwnd);
        free(buffer);
    }break;

    case WM_DESTROY:
    {
        // Only quit if no notes are left
        if(noteCount == 0)
            PostQuitMessage(0);
    }
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

//WinMain
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
   OpenDatabase();
   noteCount = getNoteCount(db);
   notes_true = malloc(sizeof(struct Note) * noteCount) ;

   char* content;
   RECT rect = { 10, 10, 210, 110 };

   sqlite3_stmt* stmt;
   const char* sql = "SELECT id, title, content FROM notes;";

   if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
       MessageBoxA(NULL, sqlite3_errmsg(db), "Failed to prepare select", MB_OK | MB_ICONERROR);
   }
   else {
       int i = 0;
       while (sqlite3_step(stmt) == SQLITE_ROW) {
           int id = sqlite3_column_int(stmt, 0);
           if (i >= noteCount) {
               // Must realloc here if you weren't relying on the initial count.
               break;
           }
           const unsigned char* title = sqlite3_column_text(stmt, 1);
           const unsigned char* content = sqlite3_column_text(stmt, 2);

           notes_true[i].id = id;
           strncpy_s(notes_true[i].title, sizeof(notes_true[i].title), (const char*)title, _TRUNCATE);
           strncpy_s(notes_true[i].text, sizeof(notes_true[i].text), (const char*)content, _TRUNCATE);
           notes_true[i].textLen = (int)strlen((const char*)content);
           notes_true[i].rect = rect;
           i++;
       }
       sqlite3_finalize(stmt);
   }
   ////////////////////////////////////
 
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

//Function definitions
////////////////////////////////
int getNoteCount(sqlite3* db) {
    sqlite3_stmt* stmt;
    int count = 0;
    const char* sql = "SELECT COUNT(*) FROM notes;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            count = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    return count;
}

int OpenDatabase(void) {
    int rc = sqlite3_open("notes.db", &db);
    if (rc != SQLITE_OK) {
        MessageBoxA(NULL, sqlite3_errmsg(db), "sqlite3_open failed", MB_OK | MB_ICONERROR);
        sqlite3_close(db);
        db = NULL;
        return rc;
    }

    const char* create_sql =
        "CREATE TABLE IF NOT EXISTS notes ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "title TEXT,"
        "content TEXT,"
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP);";

    char* errmsg = NULL;
    rc = sqlite3_exec(db, create_sql, NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        MessageBoxA(NULL, errmsg ? errmsg : "Unknown error", "sqlite3_exec failed", MB_OK | MB_ICONERROR);
        sqlite3_free(errmsg);
        sqlite3_close(db);
        db = NULL;
        return rc;
    }

  
    return SQLITE_OK;
}


int addToDatabase(struct Note* note)
{
    int rc;
    sqlite3_stmt* stmt;
    char* errmsg = NULL;

    const char* insert_sql =
        "INSERT INTO notes (title, content) VALUES (?, ?);";

    rc = sqlite3_prepare_v2(db, insert_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        MessageBoxA(NULL, sqlite3_errmsg(db), "Prepare failed", MB_OK | MB_ICONERROR);
        return rc;
    }

    sqlite3_bind_text(stmt, 1, "New note", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, note->text, -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        MessageBoxA(NULL, "Failed to insert note", "Error", MB_OK | MB_ICONERROR);
        return rc;
    }

    // Get the last inserted ID
    sqlite3_int64 last_id = sqlite3_last_insert_rowid(db);

    const char* select_sql =
        "SELECT title, content FROM notes WHERE id = ?;";

    rc = sqlite3_prepare_v2(db, select_sql, -1, &stmt, NULL);
    if (rc == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, last_id);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const unsigned char* title = sqlite3_column_text(stmt, 0);
            const unsigned char* content = sqlite3_column_text(stmt, 1);

            char message[512];
            snprintf(message, sizeof(message), "Title: %s\nContent: %s", title, content);
            //MessageBoxA(NULL, message, "Note Added", MB_OK);
        }
        sqlite3_finalize(stmt);
    }
    else {
        MessageBoxA(NULL, "Failed to prepare SELECT", "Error", MB_OK | MB_ICONERROR);
    }
    note->id = (int)last_id;
    char buffer[64];

    snprintf(
        buffer,
        sizeof(buffer),
        "The value is: %d",
        note->id
    );
    return SQLITE_OK;
}

void deleteNoteFromDatabase(int noteId) {
    if (!db) return;

    sqlite3_stmt* stmt = NULL;
    const char* sql = "DELETE FROM notes WHERE id = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        MessageBoxA(NULL, sqlite3_errmsg(db), "Prepare failed", MB_OK | MB_ICONERROR);
        return;
    }

    sqlite3_bind_int(stmt, 1, noteId);

    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        MessageBoxA(NULL, sqlite3_errmsg(db), "Delete failed", MB_OK | MB_ICONERROR);
    }

    sqlite3_finalize(stmt);
}

char* getDatabaseEntry(int noteId) {
    if (!db) {
        MessageBox(NULL, "Database not open", "Error", MB_OK | MB_ICONERROR);
        return NULL;
    }

    const char* sql = "SELECT content FROM notes WHERE id = ?;";
    sqlite3_stmt* stmt = NULL;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        MessageBoxA(NULL, sqlite3_errmsg(db), "sqlite3_prepare_v2 failed", MB_OK | MB_ICONERROR);
        return NULL;
    }

    sqlite3_bind_int(stmt, 1, noteId);

    char* result = NULL;

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        const unsigned char* text = sqlite3_column_text(stmt, 0);
        if (text) {
            result = _strdup((const char*)text);
        }
    }
    else if (rc != SQLITE_DONE) {
        MessageBoxA(NULL, sqlite3_errmsg(db), "sqlite3_step failed", MB_OK | MB_ICONERROR);
    }

    sqlite3_finalize(stmt);
    return result; // caller must free
}

void RecalculateNotePositions(HWND hwnd) {
    int x = NOTE_MARGIN;
    int y = NOTE_MARGIN;
    int totalContentHeight = 0;

    for (int i = 0; i < noteCount; i++)
    {
        notes_true[i].rect.left = x;
        notes_true[i].rect.top = y;
        notes_true[i].rect.right = x + NOTE_WIDTH;
        notes_true[i].rect.bottom = y + NOTE_HEIGHT;
        y += NOTE_HEIGHT + NOTE_MARGIN; // stack vertically
    }

    RECT client;
    GetClientRect(hwnd, &client);
    SendMessage(hwnd, WM_SIZE, 0, MAKELPARAM(client.right - client.left, client.bottom - client.top));
}

void updateDatabaseEntry(int noteId, const char* buffer) {
    sqlite3_stmt* stmt;
    int rc;
    
    const char* sql = "UPDATE notes SET title = ?, content = ? WHERE id = ?;";
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        MessageBoxA(NULL, sqlite3_errmsg(db), "Failed to prepare update", MB_OK | MB_ICONERROR);
        return;
    }

    sqlite3_bind_text(stmt, 1, "Updated Note", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, buffer, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, noteId);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        MessageBoxA(NULL, sqlite3_errmsg(db), "Update failed", MB_OK | MB_ICONERROR);
    }
    sqlite3_finalize(stmt);
}
