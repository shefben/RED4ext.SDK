// Provides emote animation lookup.
public class EmoteController {
    private static let paths: array<String> = [
        "animations/emotes/1.anim",
        "animations/emotes/2.anim",
        "animations/emotes/3.anim",
        "animations/emotes/4.anim",
        "animations/emotes/5.anim",
        "animations/emotes/6.anim",
        "animations/emotes/7.anim",
        "animations/emotes/8.anim"
    ];

    public static func GetAnim(emoteId: Uint32) -> String {
        if emoteId < ArraySize(paths) {
            return paths[emoteId];
        };
        return "";
    }
}
