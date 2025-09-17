public class BreachHud extends inkGameController {
    private let gridW: Uint8;
    private let gridH: Uint8;
    private let seed: Uint32;
    private let selections: array<Uint8>;
    private let active: Bool;
    private let timeLeft: Float;
    private let grid: wref<inkCompoundWidget>;
    private let bufferRow: wref<inkFlex>;
    private let cells: array<wref<inkCompoundWidget>>;
    private let cellStates: array<Uint8>;
    private let bufferCells: array<wref<inkTextWidget>>;
    private let codes: array<String>;
    private let timerTxt: wref<inkTextWidget>;
    private let attemptTxt: wref<inkTextWidget>;
    private let attempts: Int32;

    private const stateDefault: Uint8 = 0u;
    private const stateHover: Uint8 = 1u;
    private const stateSelected: Uint8 = 2u;
    private const stateDisabled: Uint8 = 3u;

    private func ApplyState(idx: Int32, state: Uint8) -> Void {
        cellStates[idx] = state;
        let b = cells[idx].GetWidget(n"bg") as inkBorder;
        switch state {
            case stateHover:
                b.SetTintColor(new HDRColor(0.8,0.8,0.2,1.0));
                break;
            case stateSelected:
                b.SetTintColor(new HDRColor(0.2,1.0,0.2,1.0));
                break;
            case stateDisabled:
                b.SetTintColor(new HDRColor(0.4,0.4,0.4,1.0));
                break;
            default:
                b.SetTintColor(new HDRColor(1.0,1.0,1.0,1.0));
        };
    }

    public func Start(peerId: Uint32, s: Uint32, w: Uint8, h: Uint8) -> Void {
        gridW = w;
        gridH = h;
        seed = s;
        timeLeft = 45.0;
        selections.Clear();
        attempts = 4;
        active = true;
        LogChannel(n"DEBUG", "BreachHud.Start seed=" + IntToString(Cast<Int32>(s)));
        codes.Clear();
        cells.Clear();
        bufferCells.Clear();
        cellStates.Clear();

        grid = new inkFlex();
        grid.SetName(n"grid");
        grid.SetLayoutOrientation(inkEOrientation.Vertical);
        this.GetRootCompoundWidget().AddChild(grid);

        bufferRow = new inkFlex();
        bufferRow.SetLayoutOrientation(inkEOrientation.Horizontal);
        grid.AddChild(bufferRow);

        timerTxt = new inkTextWidget();
        timerTxt.SetText("45");
        grid.AddChild(timerTxt);

        attemptTxt = new inkTextWidget();
        attemptTxt.SetText("4");
        grid.AddChild(attemptTxt);

        let rng: Uint32 = seed;
        let vals: array<String> = ["1C", "55", "BD", "E9"];
        for y in range(0, Cast<Int32>(gridH)) {
            let row = new inkFlex();
            row.SetLayoutOrientation(inkEOrientation.Horizontal);
            grid.AddChild(row);
            for x in range(0, Cast<Int32>(gridW)) {
                rng = rng * 1103515245u + 12345u;
                let code = vals[rng % ArraySize(vals)];
                codes.PushBack(code);
                // Create cell manually since custom resource may not exist
                let cell = new inkButton();
                cell.SetName(n"hex_cell");
                cell.SetSize(80.0, 80.0);
                row.AddChild(cell);
                // Create text widget for the button
                let txt = new inkText();
                txt.SetText(code);
                txt.SetFontSize(24);
                txt.SetAnchor(inkEAnchor.Centered);
                cell.AddChild(txt);
                cell.RegisterToCallback(n"OnRelease", this, n"OnCellClick");
                cell.RegisterToCallback(n"OnEnter", this, n"OnCellHover");
                cell.RegisterToCallback(n"OnLeave", this, n"OnCellUnhover");
                cells.PushBack(cell);
                cellStates.PushBack(stateDefault);
            }
        }
    }

    public func OnInput(peerId: Uint32, idx: Uint8) -> Void {
        selections.PushBack(idx);
        LogChannel(n"DEBUG", "BreachHud.Input idx=" + IntToString(Cast<Int32>(idx)));
        if peerId == Net_GetPeerId() {
            ApplyState(idx, stateSelected);
        } else {
            let b = cells[idx].GetWidget(n"bg") as inkBorder;
            b.SetTintColor(new HDRColor(0.2,0.6,1.0,1.0));
        };
        Net_SendBreachInput(idx);
    }

    protected cb func OnCellClick(widget: ref<inkWidget>) -> Bool {
        if !active { return false; };
        let idx = cells.IndexOf(widget as inkCompoundWidget);
        if idx < 0 { return false; };
        ApplyState(idx, stateSelected);
        let bufTxt = new inkTextWidget();
        bufTxt.SetText(codes[idx]);
        bufferRow.AddChild(bufTxt);
        bufferCells.PushBack(bufTxt);
        widget.UnregisterFromCallback(n"OnRelease", this, n"OnCellClick");
        OnInput(Net_GetPeerId(), Cast<Uint8>(idx));
        attempts -= 1;
        attemptTxt.SetText(IntToString(attempts));
        if attempts <= 0 {
            active = false;
            DisableAll();
        };
        return true;
    }

    protected cb func OnCellHover(widget: ref<inkWidget>) -> Bool {
        if !active { return false; };
        let idx = cells.IndexOf(widget as inkCompoundWidget);
        if idx >= 0 && cellStates[idx] == stateDefault {
            ApplyState(idx, stateHover);
        };
        return true;
    }

    protected cb func OnCellUnhover(widget: ref<inkWidget>) -> Bool {
        let idx = cells.IndexOf(widget as inkCompoundWidget);
        if idx >= 0 && cellStates[idx] == stateHover {
            ApplyState(idx, stateDefault);
        };
        return true;
    }

    public func OnUpdate(dt: Float) -> Void {
        if !active { return; };
        timeLeft -= dt;
        timerTxt.SetText(IntToString(Cast<Int32>(CeilF(timeLeft))));
        if timeLeft <= 0.0 {
            active = false;
            DisableAll();
        };
    }

    public func ShowResult(peerId: Uint32, mask: Uint8) -> Void {
        active = false;
        DisableAll();
        LogChannel(n"DEBUG", "BreachHud.Result mask=" + IntToString(Cast<Int32>(mask)));
        if mask > 0 {
            timerTxt.PlayLibraryAnimation(n"flashGreen");
        } else {
            timerTxt.PlayLibraryAnimation(n"flashRed");
        };
    }

    private func DisableAll() -> Void {
        for i in range(0, ArraySize(cells)) {
            let cell = cells[i];
            cell.UnregisterFromCallback(n"OnRelease", this, n"OnCellClick");
            cell.UnregisterFromCallback(n"OnEnter", this, n"OnCellHover");
            cell.UnregisterFromCallback(n"OnLeave", this, n"OnCellUnhover");
            ApplyState(i, stateDisabled);
        }
    }
}
