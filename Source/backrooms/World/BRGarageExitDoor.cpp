#include "World/BRGarageExitDoor.h"
#include "World/BRGarageKeyManager.h"

ABRGarageExitDoor::ABRGarageExitDoor()
{bOpenAwayFromPlayer=false;OpenAngle=-100.f;OpenDuration=1.3f;}
bool ABRGarageExitDoor::OpenAfterKeysInserted()
{
    if(!HasAuthority() || !KeyManager || !KeyManager->AreAllKeysInserted())return false;
    if(IsOpen())return true;
    SetUnlockable(true);
    return SetDoorOpen(true);
}
