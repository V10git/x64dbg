#include "Bridge.h"
#include <QClipboard>
#include "QBeaEngine.h"
#include "main.h"
#include "Exports.h"

/************************************************************************************
                            Global Variables
************************************************************************************/
static Bridge* mBridge;

/************************************************************************************
                            Class Members
************************************************************************************/
Bridge::Bridge(QObject* parent) : QObject(parent)
{
    mBridgeMutex = new QMutex();
    winId = 0;
    scriptView = 0;
    referenceManager = 0;
    bridgeResult = 0;
    hasBridgeResult = false;
    dbgStopped = false;
}

Bridge::~Bridge()
{
    delete mBridgeMutex;
}

void Bridge::CopyToClipboard(const QString & text)
{
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(text);
}

void Bridge::setResult(dsint result)
{
    bridgeResult = result;
    hasBridgeResult = true;
}

/************************************************************************************
                            Static Functions
************************************************************************************/
Bridge* Bridge::getBridge()
{
    return mBridge;
}

void Bridge::initBridge()
{
    mBridge = new Bridge();
}

/************************************************************************************
                            Helper Functions
************************************************************************************/

void Bridge::emitLoadSourceFile(const QString path, int line, int selection)
{
    emit loadSourceFile(path, line, selection);
}

void Bridge::emitMenuAddToList(QWidget* parent, QMenu* menu, int hMenu, int hParentMenu)
{
    BridgeResult result;
    emit menuAddMenuToList(parent, menu, hMenu, hParentMenu);
    result.Wait();
}

void Bridge::setDbgStopped()
{
    dbgStopped = true;
}

/************************************************************************************
                            Message processing
************************************************************************************/

void* Bridge::processMessage(GUIMSG type, void* param1, void* param2)
{
    if(dbgStopped) //there can be no more messages if the debugger stopped = BUG
        __debugbreak();
    switch(type)
    {
    case GUI_DISASSEMBLE_AT:
        emit disassembleAt((dsint)param1, (dsint)param2);
        break;

    case GUI_SET_DEBUG_STATE:
        emit dbgStateChanged((DBGSTATE)(dsint)param1);
        break;

    case GUI_ADD_MSG_TO_LOG:
        emit addMsgToLog(QString((const char*)param1));
        break;

    case GUI_CLEAR_LOG:
        emit clearLog();
        break;

    case GUI_UPDATE_REGISTER_VIEW:
        emit updateRegisters();
        break;

    case GUI_UPDATE_DISASSEMBLY_VIEW:
        emit repaintGui();
        break;

    case GUI_UPDATE_BREAKPOINTS_VIEW:
        emit updateBreakpoints();
        break;

    case GUI_UPDATE_WINDOW_TITLE:
        emit updateWindowTitle(QString((const char*)param1));
        break;

    case GUI_GET_WINDOW_HANDLE:
        return winId;

    case GUI_DUMP_AT:
        emit dumpAt((dsint)param1);
        break;

    case GUI_SCRIPT_ADD:
    {
        BridgeResult result;
        emit scriptAdd((int)param1, (const char**)param2);
        result.Wait();
    }
    break;

    case GUI_SCRIPT_CLEAR:
        emit scriptClear();
        break;

    case GUI_SCRIPT_SETIP:
        emit scriptSetIp((int)param1);
        break;

    case GUI_SCRIPT_ERROR:
    {
        BridgeResult result;
        emit scriptError((int)param1, QString((const char*)param2));
        result.Wait();
    }
    break;

    case GUI_SCRIPT_SETTITLE:
        emit scriptSetTitle(QString((const char*)param1));
        break;

    case GUI_SCRIPT_SETINFOLINE:
        emit scriptSetInfoLine((int)param1, QString((const char*)param2));
        break;

    case GUI_SCRIPT_MESSAGE:
    {
        BridgeResult result;
        emit scriptMessage(QString((const char*)param1));
        result.Wait();
    }
    break;

    case GUI_SCRIPT_MSGYN:
    {
        BridgeResult result;
        emit scriptQuestion(QString((const char*)param1));
        return (void*)result.Wait();
    }
    break;

    case GUI_SCRIPT_ENABLEHIGHLIGHTING:
        emit scriptEnableHighlighting((bool)param1);
        break;

    case GUI_SYMBOL_UPDATE_MODULE_LIST:
        emit updateSymbolList((int)param1, (SYMBOLMODULEINFO*)param2);
        break;

    case GUI_SYMBOL_LOG_ADD:
        emit addMsgToSymbolLog(QString((const char*)param1));
        break;

    case GUI_SYMBOL_LOG_CLEAR:
        emit clearSymbolLog();
        break;

    case GUI_SYMBOL_SET_PROGRESS:
        emit setSymbolProgress((int)param1);
        break;

    case GUI_REF_ADDCOLUMN:
        emit referenceAddColumnAt((int)param1, QString((const char*)param2));
        break;

    case GUI_REF_SETROWCOUNT:
        emit referenceSetRowCount((dsint)param1);
        break;

    case GUI_REF_GETROWCOUNT:
        return (void*)referenceManager->currentReferenceView()->mList->getRowCount();

    case GUI_REF_DELETEALLCOLUMNS:
        GuiReferenceInitialize("References");
        break;

    case GUI_REF_SETCELLCONTENT:
    {
        CELLINFO* info = (CELLINFO*)param1;
        emit referenceSetCellContent(info->row, info->col, QString(info->str));
    }
    break;

    case GUI_REF_GETCELLCONTENT:
        return (void*)referenceManager->currentReferenceView()->mList->getCellContent((int)param1, (int)param2).toUtf8().constData();

    case GUI_REF_RELOADDATA:
        emit referenceReloadData();
        break;

    case GUI_REF_SETSINGLESELECTION:
        emit referenceSetSingleSelection((int)param1, (bool)param2);
        break;

    case GUI_REF_SETPROGRESS:
        emit referenceSetProgress((int)param1);
        break;

    case GUI_REF_SETCURRENTTASKPROGRESS:
        emit referenceSetCurrentTaskProgress((int)param1, QString((const char*)param2));
        break;

    case GUI_REF_SETSEARCHSTARTCOL:
        emit referenceSetSearchStartCol((int)param1);
        break;

    case GUI_REF_INITIALIZE:
    {
        BridgeResult result;
        emit referenceInitialize(QString((const char*)param1));
        result.Wait();
    }
    break;

    case GUI_STACK_DUMP_AT:
        emit stackDumpAt((duint)param1, (duint)param2);
        break;

    case GUI_UPDATE_DUMP_VIEW:
        emit updateDump();
        break;

    case GUI_UPDATE_THREAD_VIEW:
        emit updateThreads();
        break;

    case GUI_UPDATE_MEMORY_VIEW:
        emit updateMemory();
        break;

    case GUI_ADD_RECENT_FILE:
        emit addRecentFile(QString((const char*)param1));
        break;

    case GUI_SET_LAST_EXCEPTION:
        emit setLastException((unsigned int)param1);
        break;

    case GUI_GET_DISASSEMBLY:
    {
        duint parVA = (duint)param1;
        char* text = (char*)param2;
        if(!text || !parVA || !DbgIsDebugging())
            return 0;
        byte_t wBuffer[16];
        if(!DbgMemRead(parVA, wBuffer, 16))
            return 0;
        QBeaEngine disasm(-1);
        Instruction_t instr = disasm.DisassembleAt(wBuffer, 16, 0, 0, parVA);
        QList<RichTextPainter::CustomRichText_t> richText;
        CapstoneTokenizer::TokenToRichText(instr.tokens, richText, 0);
        QString finalInstruction = "";
        for(int i = 0; i < richText.size(); i++)
            finalInstruction += richText.at(i).text;
        strcpy_s(text, GUI_MAX_DISASSEMBLY_SIZE, finalInstruction.toUtf8().constData());
        return (void*)1;
    }
    break;

    case GUI_MENU_ADD:
    {
        BridgeResult result;
        emit menuAddMenu((int)param1, QString((const char*)param2));
        return (void*)result.Wait();
    }
    break;

    case GUI_MENU_ADD_ENTRY:
    {
        BridgeResult result;
        emit menuAddMenuEntry((int)param1, QString((const char*)param2));
        return (void*)result.Wait();
    }
    break;

    case GUI_MENU_ADD_SEPARATOR:
    {
        BridgeResult result;
        emit menuAddSeparator((int)param1);
        result.Wait();
    }
    break;

    case GUI_MENU_CLEAR:
    {
        BridgeResult result;
        emit menuClearMenu((int)param1);
        result.Wait();
    }
    break;

    case GUI_SELECTION_GET:
    {
        int hWindow = (int)param1;
        SELECTIONDATA* selection = (SELECTIONDATA*)param2;
        if(!DbgIsDebugging())
            return (void*)false;
        BridgeResult result;
        switch(hWindow)
        {
        case GUI_DISASSEMBLY:
            emit selectionDisasmGet(selection);
            break;
        case GUI_DUMP:
            emit selectionDumpGet(selection);
            break;
        case GUI_STACK:
            emit selectionStackGet(selection);
            break;
        default:
            return (void*)false;
        }
        result.Wait();
        if(selection->start > selection->end) //swap start and end
        {
            dsint temp = selection->end;
            selection->end = selection->start;
            selection->start = temp;
        }
        return (void*)true;
    }
    break;

    case GUI_SELECTION_SET:
    {
        int hWindow = (int)param1;
        const SELECTIONDATA* selection = (const SELECTIONDATA*)param2;
        if(!DbgIsDebugging())
            return (void*)false;
        BridgeResult result;
        switch(hWindow)
        {
        case GUI_DISASSEMBLY:
            emit selectionDisasmSet(selection);
            break;
        case GUI_DUMP:
            emit selectionDumpSet(selection);
            break;
        case GUI_STACK:
            emit selectionStackSet(selection);
            break;
        default:
            return (void*)false;
        }
        return (void*)result.Wait();
    }
    break;

    case GUI_GETLINE_WINDOW:
    {
        QString text = "";
        BridgeResult result;
        emit getStrWindow(QString((const char*)param1), &text);
        if(result.Wait())
        {
            strcpy_s((char*)param2, GUI_MAX_LINE_SIZE, text.toUtf8().constData());
            return (void*)true;
        }
        return (void*)false; //cancel/escape
    }
    break;

    case GUI_AUTOCOMPLETE_ADDCMD:
        emit autoCompleteAddCmd(QString((const char*)param1));
        break;

    case GUI_AUTOCOMPLETE_DELCMD:
        emit autoCompleteDelCmd(QString((const char*)param1));
        break;

    case GUI_AUTOCOMPLETE_CLEARALL:
        emit autoCompleteClearAll();
        break;

    case GUI_ADD_MSG_TO_STATUSBAR:
        emit addMsgToStatusBar(QString((const char*)param1));
        break;

    case GUI_UPDATE_SIDEBAR:
        emit updateSideBar();
        break;

    case GUI_REPAINT_TABLE_VIEW:
        emit repaintTableView();
        break;

    case GUI_UPDATE_PATCHES:
        emit updatePatches();
        break;

    case GUI_UPDATE_CALLSTACK:
        emit updateCallStack();
        break;

    case GUI_SYMBOL_REFRESH_CURRENT:
        emit symbolRefreshCurrent();
        break;

    case GUI_LOAD_SOURCE_FILE:
        emitLoadSourceFile(QString((const char*)param1), (int)param2);
        break;

    case GUI_MENU_SET_ICON:
    {
        int hMenu = (int)param1;
        const ICONDATA* icon = (const ICONDATA*)param2;
        BridgeResult result;
        if(!icon)
            emit setIconMenu(hMenu, QIcon());
        else
        {
            QImage img;
            img.loadFromData((uchar*)icon->data, icon->size);
            QIcon qIcon(QPixmap::fromImage(img));
            emit setIconMenu(hMenu, qIcon);
        }
        result.Wait();
    }
    break;

    case GUI_MENU_SET_ENTRY_ICON:
    {
        int hEntry = (int)param1;
        const ICONDATA* icon = (const ICONDATA*)param2;
        BridgeResult result;
        if(!icon)
            emit setIconMenuEntry(hEntry, QIcon());
        else
        {
            QImage img;
            img.loadFromData((uchar*)icon->data, icon->size);
            QIcon qIcon(QPixmap::fromImage(img));
            emit setIconMenuEntry(hEntry, qIcon);
        }
        result.Wait();
    }
    break;

    case GUI_SHOW_CPU:
        emit showCpu();
        break;

    case GUI_ADD_QWIDGET_TAB:
        emit addQWidgetTab((QWidget*)param1);
        break;

    case GUI_SHOW_QWIDGET_TAB:
        emit showQWidgetTab((QWidget*)param1);
        break;

    case GUI_CLOSE_QWIDGET_TAB:
        emit closeQWidgetTab((QWidget*)param1);
        break;

    case GUI_EXECUTE_ON_GUI_THREAD:
        GuiAddLogMessage(QString().sprintf("thread id (bridge) %X\n", GetCurrentThreadId()).toUtf8().constData());
        emit executeOnGuiThread(param1);
        break;

    case GUI_UPDATE_TIME_WASTED_COUNTER:
        emit updateTimeWastedCounter();
        break;

    case GUI_SET_GLOBAL_NOTES:
    {
        QString text = QString((const char*)param1);
        emit setGlobalNotes(text);
    }
    break;

    case GUI_GET_GLOBAL_NOTES:
    {
        BridgeResult result;
        emit getGlobalNotes(param1);
        result.Wait();
    }
    break;

    case GUI_SET_DEBUGGEE_NOTES:
    {
        QString text = QString((const char*)param1);
        emit setDebuggeeNotes(text);
    }
    break;

    case GUI_GET_DEBUGGEE_NOTES:
    {
        BridgeResult result;
        emit getDebuggeeNotes(param1);
        result.Wait();
    }
    break;

    case GUI_DUMP_AT_N:
        emit dumpAtN((duint)param1, (int)param2);
        break;

    case GUI_DISPLAY_WARNING:
    {
        QString title = QString((const char*)param1);
        QString text = QString((const char*)param2);
        emit displayWarning(title, text);
    }
    break;

    case GUI_REGISTER_SCRIPT_LANG:
    {
        BridgeResult result;
        emit registerScriptLang((SCRIPTTYPEINFO*)param1);
        result.Wait();
    }
    break;

    case GUI_UNREGISTER_SCRIPT_LANG:
        emit unregisterScriptLang((int)param1);
        break;
    }
    return nullptr;
}

/************************************************************************************
                            Exported Functions
************************************************************************************/
__declspec(dllexport) int _gui_guiinit(int argc, char* argv[])
{
    return main(argc, argv);
}

__declspec(dllexport) void* _gui_sendmessage(GUIMSG type, void* param1, void* param2)
{
    return Bridge::getBridge()->processMessage(type, param1, param2);
}
