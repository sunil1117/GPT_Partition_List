/** @file
  Clovertrail I2C test application

  This application tests "UEFI Entropy-Gathering Protocol" 

Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.

**/
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>
#include <Protocol/DiskIo.h>
#include <Protocol/BlockIo.h>

extern EFI_BOOT_SERVICES        *gBS;


INTN
EFIAPI
ShellAppMain (
  IN UINTN Argc,
  IN CHAR16 **Argv
  )
{
  EFI_STATUS                        Status;
  UINTN		                        NumHandles;
  EFI_HANDLE	                    *HandleBuffer;
  EFI_BLOCK_IO_PROTOCOL             *BlockIo;
  EFI_DISK_IO_PROTOCOL              *DiskIo;
  EFI_PARTITION_TABLE_HEADER        *PartHdr;
  EFI_PARTITION_ENTRY               *PartEntry;
  EFI_PARTITION_ENTRY               *Entry;
  UINT32                            MediaId;
  UINT32                            BlockSize;
  UINTN		                        i,Index;

  Status = EFI_SUCCESS;

  Status = gBS->LocateHandleBuffer(ByProtocol,&gEfiDiskIoProtocolGuid,NULL,&NumHandles,&HandleBuffer);
  if (EFI_ERROR(Status)){
    Print(L"Fail to locate Disk io\n");
    return Status;
  }

   Print(L"Find %x handle with DiskIo\n",NumHandles);


  for (i = 0; i< NumHandles; ++i)
  {
	gBS->HandleProtocol(HandleBuffer[i],&gEfiBlockIoProtocolGuid,&BlockIo);
	if(BlockIo->Media->RemovableMedia){
      Print(L"Find remove device %x, skip \n",i);
      continue;

	}
      Print(L"Find unremove device %x\n",i);
      break;
  }

  Print(L"To Locate DiskIo\n");


  Status = gBS->HandleProtocol(HandleBuffer[i],&gEfiDiskIoProtocolGuid, &DiskIo);
  if(!EFI_ERROR(Status)) {

    Print(L"To Read PRIMARY_PART_HEADER_LBA\n");


  	BlockSize = BlockIo->Media->BlockSize;
    MediaId   = BlockIo->Media->MediaId;
    PartHdr   = AllocateZeroPool (BlockSize);


    Status = DiskIo->ReadDisk (
                     DiskIo,
                     MediaId,
                     MultU64x32 (PRIMARY_PART_HEADER_LBA, BlockSize),
                     BlockSize,
                     PartHdr
                     );
    if(!EFI_ERROR(Status)) {
    	if ((PartHdr->Header.Signature != EFI_PTAB_HEADER_ID) ||PartHdr->MyLBA != PRIMARY_PART_HEADER_LBA 
    		||(PartHdr->SizeOfPartitionEntry < sizeof (EFI_PARTITION_ENTRY))) 
    	{
    		Print (L"Invalid efi partition table header\n");
    		FreePool (PartHdr);
    		Status = EFI_LOAD_ERROR;
    		return Status;
    	}

    	Print(L"To Read patition entrys\n");

    	PartEntry = AllocatePool (PartHdr->NumberOfPartitionEntries * PartHdr->SizeOfPartitionEntry);
    	if (PartEntry == NULL) {
    		Print(L"Allocate pool error\n");
    		Status = EFI_BUFFER_TOO_SMALL;
    		return Status;
    	}

        Print(L"BlockSize                :%x\n",BlockSize);
    	Print(L"PartitionEntryLBA        :%x\n",PartHdr->PartitionEntryLBA);
    	Print(L"NumberOfPartitionEntries :%x\n",PartHdr->NumberOfPartitionEntries);
    	Print(L"SizeOfPartitionEntry     :%x\n",PartHdr->SizeOfPartitionEntry);




    	Status = DiskIo->ReadDisk (
                     DiskIo,
                     MediaId,
                     MultU64x32(PartHdr->PartitionEntryLBA, BlockSize),
                     PartHdr->NumberOfPartitionEntries * (PartHdr->SizeOfPartitionEntry),
                     PartEntry
                     );
    	if (EFI_ERROR (Status)) {
    		Print(L" Partition Entry ReadDisk error\n");
    		Status = EFI_DEVICE_ERROR;
    		return Status;
    	}

        for (Index = 0; Index < PartHdr->NumberOfPartitionEntries; Index++) {
        	Entry = (EFI_PARTITION_ENTRY *) ((UINT8 *) PartEntry + Index * PartHdr->SizeOfPartitionEntry);
        	if(Entry->StartingLBA==0&&Entry->EndingLBA==0) break;
        	Print(L" Partition            :%d\n",Index);
        	Print(L" PartitionTypeGUID    :%g\n",Entry->PartitionTypeGUID);
        	Print(L" UniquePartitionGUID  :%g\n",Entry->UniquePartitionGUID);
        	Print(L" StartingLBA          :%d\n",Entry->StartingLBA);
        	Print(L" EndingLBA            :%d\n",Entry->EndingLBA);

        }
    }
  }

  return Status;
}
