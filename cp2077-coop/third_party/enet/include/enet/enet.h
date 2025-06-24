#ifndef ENET_ENET_H
#define ENET_ENET_H

/* Placeholder ENet header for build stubs */
struct ENetHost {};
struct ENetPeer {};
typedef struct ENetAddress {} ENetAddress;
typedef enum ENetEventType { ENET_EVENT_TYPE_NONE = 0, ENET_EVENT_TYPE_CONNECT, ENET_EVENT_TYPE_DISCONNECT, ENET_EVENT_TYPE_RECEIVE } ENetEventType;
typedef struct ENetPacket { void* data; size_t dataLength; } ENetPacket;
typedef struct ENetEvent { ENetEventType type; ENetPeer* peer; ENetPacket* packet; } ENetEvent;
ENetHost* enet_host_create(const ENetAddress*, size_t, size_t, unsigned int, unsigned int);
void enet_host_destroy(ENetHost*);
int enet_host_service(ENetHost*, ENetEvent*, unsigned int);
int enet_initialize(void);
void enet_deinitialize(void);

#endif /* ENET_ENET_H */
