"""Public plugin API (v1)."""

API_VERSION = 1

import game


def get_version() -> int:
    """Return API version number."""
    return API_VERSION


def register_packet(name: str, callback):
    """Register a custom network packet handler.

    Returns the packet ID to send with.
    """
    return game._register_packet(name, callback)


def register_panel(name: str, callback) -> None:
    """Register a UI panel factory callable."""
    game._register_panel(name, callback)
