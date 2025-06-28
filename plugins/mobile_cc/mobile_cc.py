__plugin__ = {"name": "MobileCommandCenter", "version": "1.0"}

import game

@game.register_command("spawncc", "Spawn command center")
def spawncc(peer, args):
    game.spawn_vehicle("veh_commandcenter", (0,0,0), (0,0,0,1))

