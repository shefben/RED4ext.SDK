__plugin__ = {"name": "PopupZone", "version": "1.0"}

import game

@game.on("OnTick")
def _(_: float):
    for peer, pos in game.get_peer_positions():
        if game.dist(pos, (123, 456, 0)) < 2:
            game.show_popup(peer, "Welcome to HexBar!", 5)
