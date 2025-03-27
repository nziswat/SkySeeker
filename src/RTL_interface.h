#pragma once
//might be useless tbh
#include "src/message_handler.h"

static bool exitDriverThread;

void calculateMag();
void detectModeS(uint16_t* m, uint32_t mlen);
int modesMessageLenByType(int type);
uint32_t modesChecksum(unsigned char* msg, int bits);
void decodeModesMessage(struct modesMessage* mm, unsigned char* msg);
int decodeAC12Field(unsigned char* msg, int* unit);
int decodeAC13Field(unsigned char* msg, int* unit);
void displayModesMessage(struct modesMessage* mm);
void sendModesData(modesMessage& mm);
int runRTL(MessageHandler* NewMessageHandler);
void testLoop(MessageHandler* NewMessageHandler);