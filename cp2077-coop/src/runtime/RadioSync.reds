public class RadioSync {
    public static func OnChange(pkt: ref<RadioChangePacket>) -> Void {
        let vehSys = GameInstance.GetVehicleSystem(GetGame());
        let veh = vehSys.FindVehicleByID(pkt.vehId);
        if IsDefined(veh) {
            RED4ext.ExecuteFunction("VehicleRadioSystem", "SetStation", veh, pkt.stationId);
            RED4ext.ExecuteFunction("VehicleRadioSystem", "SeekTime", veh, Cast<Float>(pkt.offsetSec));
        };
    }
}

public static func RadioSync_OnChange(pkt: ref<RadioChangePacket>) -> Void {
    RadioSync.OnChange(pkt);
}
