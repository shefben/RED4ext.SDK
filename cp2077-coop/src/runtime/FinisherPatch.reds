@wrapMethod(FinisherSystem)
public final func StartFinisher(actor: wref<GameObject>, victim: wref<GameObject>) -> Void {
    wrappedMethod(actor, victim);
    if Net_IsAuthoritative() {
        let ts = GameInstance.GetTransactionSystem(GetGame());
        let item: ItemID = ts.GetItemInSlot(actor, t"RightHand");
        let type: gamedataItemCategory = RPGManager.GetItemCategory(ItemID.GetTDBID(item));
        if type != gamedataItemCategory.Katana {
            let ft: Uint8 = 0u;
            switch type {
                case gamedataItemCategory.Pistol:
                    ft = 1u;
                    break;
                case gamedataItemCategory.Knife:
                    ft = 2u;
                    break;
                case gamedataItemCategory.Revolver:
                    ft = 3u;
                    break;
                default:
                    ft = 0u;
            };
            if ft > 0u {
                Net_BroadcastFinisherStart(EntityID.GetHash(actor.GetEntityID()), EntityID.GetHash(victim.GetEntityID()), ft);
                Net_BroadcastSlowMoFinisher(Net_GetPeerId(), EntityID.GetHash(victim.GetEntityID()), 1500u);
            };
        };
    };
}
