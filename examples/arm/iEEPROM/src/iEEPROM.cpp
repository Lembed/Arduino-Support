#include "IEEPROM.h"
#include "libmaple/util.h"
#include "libmaple/flash.h"
// See http://www.st.com/web/en/resource/technical/document/application_note/CD00165693.pdf

/**
  * @brief  Check page for blank
  * @param  page base address
  * @retval Success or error
  *		EEPROM_BAD_FLASH:	page not empty after erase
  *		EEPROM_OK:			page blank
  */
uint16 IEEPROMClass::EE_CheckPage(uint32 pageBase, uint16 status)
{
	uint32 pageEnd = pageBase + (uint32)PageSize;

	// Page Status not EEPROM_ERASED and not a "state"
	if ((*(__io uint16*)pageBase) != EEPROM_ERASED && (*(__io uint16*)pageBase) != status) {
		return EEPROM_BAD_FLASH;
	}
	for (pageBase += 4; pageBase < pageEnd; pageBase += 4) {
		// Verify if slot is empty
		if ((*(__io uint32*)pageBase) != 0xFFFFFFFF) {
			return EEPROM_BAD_FLASH;
		}
	}
	return EEPROM_OK;
}

/**
  * @brief  Erase page with increment erase counter (page + 2)
  * @param  page base address
  * @retval Success or error
  *			FLASH_COMPLETE: success erase
  *			- Flash error code: on write Flash error
  */
FLASH_Status IEEPROMClass::EE_ErasePage(uint32 pageBase)
{
	FLASH_Status FlashStatus;
	uint16 data = (*(__io uint16*)(pageBase));
	if ((data == EEPROM_ERASED) || (data == EEPROM_VALID_PAGE) || (data == EEPROM_RECEIVE_DATA)) {
		data = (*(__io uint16*)(pageBase + 2)) + 1;
	} else {
		data = 0;
	}

	FlashStatus = FLASH_ErasePage(pageBase);
	if (FlashStatus == FLASH_COMPLETE) {
		FlashStatus = FLASH_ProgramHalfWord(pageBase + 2, data);
	}

	return FlashStatus;
}

/**
  * @brief  Check page for blank and erase it
  * @param  page base address
  * @retval Success or error
  *			- Flash error code: on write Flash error
  *			- EEPROM_BAD_FLASH:	page not empty after erase
  *			- EEPROM_OK:			page blank
  */
uint16 IEEPROMClass::EE_CheckErasePage(uint32 pageBase, uint16 status)
{
	uint16 FlashStatus;
	if (EE_CheckPage(pageBase, status) != EEPROM_OK) {
		FlashStatus = EE_ErasePage(pageBase);
		if (FlashStatus != FLASH_COMPLETE) {
			return FlashStatus;
		}
		return EE_CheckPage(pageBase, status);
	}
	return EEPROM_OK;
}

/**
  * @brief  Find valid Page for write or read operation
  * @param	Page0: Page0 base address
  *			Page1: Page1 base address
  * @retval Valid page address (PAGE0 or PAGE1) or NULL in case of no valid page was found
  */
uint32 IEEPROMClass::EE_FindValidPage(void)
{
	uint16 status0 = (*(__io uint16*)PageBase0);		// Get Page0 actual status
	uint16 status1 = (*(__io uint16*)PageBase1);		// Get Page1 actual status

	if (status0 == EEPROM_VALID_PAGE && status1 == EEPROM_ERASED) {
		return PageBase0;
	}
	if (status1 == EEPROM_VALID_PAGE && status0 == EEPROM_ERASED) {
		return PageBase1;
	}

	return 0;
}

/**
  * @brief  Calculate unique variables in EEPROM
  * @param  start: address of first slot to check (page + 4)
  * @param	end: page end address
  * @param	address: 16 bit virtual address of the variable to excluse (or 0XFFFF)
  * @retval count of variables
  */
uint16 IEEPROMClass::EE_GetVariablesCount(uint32 pageBase, uint16 skipAddress)
{
	uint16 varAddress, nextAddress;
	uint32 idx;
	uint32 pageEnd = pageBase + (uint32)PageSize;
	uint16 count = 0;

	for (pageBase += 6; pageBase < pageEnd; pageBase += 4) {
		varAddress = (*(__io uint16*)pageBase);
		if (varAddress == 0xFFFF || varAddress == skipAddress) {
			continue;
		}

		count++;
		for (idx = pageBase + 4; idx < pageEnd; idx += 4) {
			nextAddress = (*(__io uint16*)idx);
			if (nextAddress == varAddress) {
				count--;
				break;
			}
		}
	}
	return count;
}

/**
  * @brief  Transfers last updated variables data from the full Page to an empty one.
  * @param  newPage: new page base address
  * @param	oldPage: old page base address
  *	@param	SkipAddress: 16 bit virtual address of the variable (or 0xFFFF)
  * @retval Success or error status:
  *           - FLASH_COMPLETE: on success
  *           - EEPROM_OUT_SIZE: if valid new page is full
  *           - Flash error code: on write Flash error
  */
uint16 IEEPROMClass::EE_PageTransfer(uint32 newPage, uint32 oldPage, uint16 SkipAddress)
{
	uint32 oldEnd, newEnd;
	uint32 oldIdx, newIdx, idx;
	uint16 address, data, found;
	FLASH_Status FlashStatus;

	// Transfer process: transfer variables from old to the new active page
	newEnd = newPage + ((uint32)PageSize);

	// Find first free element in new page
	for (newIdx = newPage + 4; newIdx < newEnd; newIdx += 4) {
		// Verify if element
		if ((*(__io uint32*)newIdx) == 0xFFFFFFFF) {
			//  contents are 0xFFFFFFFF
			break;
		}
	}
	if (newIdx >= newEnd) {
		return EEPROM_OUT_SIZE;
	}

	oldEnd = oldPage + 4;
	oldIdx = oldPage + (uint32)(PageSize - 2);

	for (; oldIdx > oldEnd; oldIdx -= 4) {
		address = *(__io uint16*)oldIdx;
		if (address == 0xFFFF || address == SkipAddress) {
			// it's means that power off after write data
			continue;
		}

		found = 0;
		for (idx = newPage + 6; idx < newIdx; idx += 4) {
			if ((*(__io uint16*)(idx)) == address) {
				found = 1;
				break;
			}
		}

		if (found) {
			continue;
		}

		if (newIdx < newEnd) {
			data = (*(__io uint16*)(oldIdx - 2));

			FlashStatus = FLASH_ProgramHalfWord(newIdx, data);
			if (FlashStatus != FLASH_COMPLETE) {
				return FlashStatus;
			}

			FlashStatus = FLASH_ProgramHalfWord(newIdx + 2, address);
			if (FlashStatus != FLASH_COMPLETE) {
				return FlashStatus;
			}

			newIdx += 4;
		} else {
			return EEPROM_OUT_SIZE;
		}
	}

	// Erase the old Page: Set old Page status to EEPROM_EEPROM_ERASED status
	data = EE_CheckErasePage(oldPage, EEPROM_ERASED);
	if (data != EEPROM_OK) {
		return data;
	}

	// Set new Page status
	FlashStatus = FLASH_ProgramHalfWord(newPage, EEPROM_VALID_PAGE);
	if (FlashStatus != FLASH_COMPLETE) {
		return FlashStatus;
	}

	return EEPROM_OK;
}

/**
  * @brief  Verify if active page is full and Writes variable in EEPROM.
  * @param  Address: 16 bit virtual address of the variable
  * @param  Data: 16 bit data to be written as variable value
  * @retval Success or error status:
  *           - FLASH_COMPLETE: on success
  *           - EEPROM_PAGE_FULL: if valid page is full (need page transfer)
  *           - EEPROM_NO_VALID_PAGE: if no valid page was found
  *           - EEPROM_OUT_SIZE: if EEPROM size exceeded
  *           - Flash error code: on write Flash error
  */
uint16 IEEPROMClass::EE_VerifyPageFullWriteVariable(uint16 Address, uint16 Data)
{
	FLASH_Status FlashStatus;
	uint32 idx, pageBase, pageEnd, newPage;
	uint16 count;

	// Get valid Page for write operation
	pageBase = EE_FindValidPage();
	if (pageBase == 0) {
		return  EEPROM_NO_VALID_PAGE;
	}

	// Get the valid Page end Address
	pageEnd = pageBase + PageSize;			// Set end of page

	for (idx = pageEnd - 2; idx > pageBase; idx -= 4) {
		if ((*(__io uint16*)idx) == Address) {	// Find last value for address
			count = (*(__io uint16*)(idx - 2));	// Read last data
			if (count == Data) {
				return EEPROM_OK;
			}
			if (count == 0xFFFF) {
				FlashStatus = FLASH_ProgramHalfWord(idx - 2, Data);	// Set variable data
				if (FlashStatus == FLASH_COMPLETE) {
					return EEPROM_OK;
				}
			}
			break;
		}
	}

	// Check each active page address starting from begining
	for (idx = pageBase + 4; idx < pageEnd; idx += 4)
		if ((*(__io uint32*)idx) == 0xFFFFFFFF) {			// Verify if element
			//  contents are 0xFFFFFFFF
			FlashStatus = FLASH_ProgramHalfWord(idx, Data);	// Set variable data
			if (FlashStatus != FLASH_COMPLETE) {
				return FlashStatus;
			}
			FlashStatus = FLASH_ProgramHalfWord(idx + 2, Address);	// Set variable virtual address
			if (FlashStatus != FLASH_COMPLETE) {
				return FlashStatus;
			}
			return EEPROM_OK;
		}

	// Empty slot not found, need page transfer
	// Calculate unique variables in page
	count = EE_GetVariablesCount(pageBase, Address) + 1;
	if (count >= (PageSize / 4 - 1)) {
		return EEPROM_OUT_SIZE;
	}

	if (pageBase == PageBase1) {
		newPage = PageBase0;		// New page address where variable will be moved to
	} else {
		newPage = PageBase1;
	}

	// Set the new Page status to RECEIVE_DATA status
	FlashStatus = FLASH_ProgramHalfWord(newPage, EEPROM_RECEIVE_DATA);
	if (FlashStatus != FLASH_COMPLETE) {
		return FlashStatus;
	}

	// Write the variable passed as parameter in the new active page
	FlashStatus = FLASH_ProgramHalfWord(newPage + 4, Data);
	if (FlashStatus != FLASH_COMPLETE) {
		return FlashStatus;
	}

	FlashStatus = FLASH_ProgramHalfWord(newPage + 6, Address);
	if (FlashStatus != FLASH_COMPLETE) {
		return FlashStatus;
	}

	return EE_PageTransfer(newPage, pageBase, Address);
}

IEEPROMClass::iEEPROMClass(void)
{
	PageBase0 = EEPROM_PAGE0_BASE;
	PageBase1 = EEPROM_PAGE1_BASE;
	PageSize = EEPROM_PAGE_SIZE;
	Status = EEPROM_NOT_INIT;
}

uint16 IEEPROMClass::init(uint32 pageBase0, uint32 pageBase1, uint32 pageSize)
{
	PageBase0 = pageBase0;
	PageBase1 = pageBase1;
	PageSize = pageSize;
	return init();
}

uint16 IEEPROMClass::init(void)
{
	uint16 status0, status1;
	FLASH_Status FlashStatus;

	FLASH_Unlock();
	Status = EEPROM_NO_VALID_PAGE;

	status0 = (*(__io uint16 *)PageBase0);
	status1 = (*(__io uint16 *)PageBase1);

	switch (status0) {
	/*
			Page0				Page1
			-----				-----
		EEPROM_ERASED		EEPROM_VALID_PAGE			Page1 valid, Page0 erased
							EEPROM_RECEIVE_DATA			Page1 need set to valid, Page0 erased
							EEPROM_ERASED				make EE_Format
							any							Error: EEPROM_NO_VALID_PAGE
	*/
	case EEPROM_ERASED:
		// Page0 erased, Page1 valid
		if (status1 == EEPROM_VALID_PAGE)	{
			Status = EE_CheckErasePage(PageBase0, EEPROM_ERASED);
		} else if (status1 == EEPROM_RECEIVE_DATA) {	// Page0 erased, Page1 receive
			FlashStatus = FLASH_ProgramHalfWord(PageBase1, EEPROM_VALID_PAGE);
			if (FlashStatus != FLASH_COMPLETE) {
				Status = FlashStatus;
			} else {
				Status = EE_CheckErasePage(PageBase0, EEPROM_ERASED);
			}
		} else if (status1 == EEPROM_ERASED) {
			// Both in erased state so format EEPROM
			Status = format();
		}
		break;
	/*
			Page0				Page1
			-----				-----
		EEPROM_RECEIVE_DATA	EEPROM_VALID_PAGE			Transfer Page1 to Page0
							EEPROM_ERASED				Page0 need set to valid, Page1 erased
							any							EEPROM_NO_VALID_PAGE
	*/
	case EEPROM_RECEIVE_DATA:
		if (status1 == EEPROM_VALID_PAGE) {
			// Page0 receive, Page1 valid
			Status = EE_PageTransfer(PageBase0, PageBase1, 0xFFFF);
		} else if (status1 == EEPROM_ERASED) {		// Page0 receive, Page1 erased
			Status = EE_CheckErasePage(PageBase1, EEPROM_ERASED);
			if (Status == EEPROM_OK) {
				FlashStatus = FLASH_ProgramHalfWord(PageBase0, EEPROM_VALID_PAGE);
				if (FlashStatus != FLASH_COMPLETE) {
					Status = FlashStatus;
				} else {
					Status = EEPROM_OK;
				}
			}
		}
		break;
	/*
			Page0				Page1
			-----				-----
		EEPROM_VALID_PAGE	EEPROM_VALID_PAGE			Error: EEPROM_NO_VALID_PAGE
							EEPROM_RECEIVE_DATA			Transfer Page0 to Page1
							any							Page0 valid, Page1 erased
	*/
	case EEPROM_VALID_PAGE:
		if (status1 == EEPROM_VALID_PAGE)	{		// Both pages valid
			Status = EEPROM_NO_VALID_PAGE;
		} else if (status1 == EEPROM_RECEIVE_DATA) {
			Status = EE_PageTransfer(PageBase1, PageBase0, 0xFFFF);
		} else {
			Status = EE_CheckErasePage(PageBase1, EEPROM_ERASED);
		}
		break;
	/*
			Page0				Page1
			-----				-----
			any				EEPROM_VALID_PAGE			Page1 valid, Page0 erased
							EEPROM_RECEIVE_DATA			Page1 valid, Page0 erased
							any							EEPROM_NO_VALID_PAGE
	*/
	default:
		if (status1 == EEPROM_VALID_PAGE) {
			Status = EE_CheckErasePage(PageBase0, EEPROM_ERASED);	// Check/Erase Page0
		} else if (status1 == EEPROM_RECEIVE_DATA) {
			FlashStatus = FLASH_ProgramHalfWord(PageBase1, EEPROM_VALID_PAGE);
			if (FlashStatus != FLASH_COMPLETE) {
				Status = FlashStatus;
			} else {
				Status = EE_CheckErasePage(PageBase0, EEPROM_ERASED);
			}
		}
		break;
	}
	return Status;
}

/**
  * @brief  Erases PAGE0 and PAGE1 and writes EEPROM_VALID_PAGE / 0 header to PAGE0
  * @param  PAGE0 and PAGE1 base addresses
  * @retval Status of the last operation (Flash write or erase) done during EEPROM formating
  */
uint16 IEEPROMClass::format(void)
{
	uint16 status;
	FLASH_Status FlashStatus;

	FLASH_Unlock();

	// Erase Page0
	status = EE_CheckErasePage(PageBase0, EEPROM_VALID_PAGE);
	if (status != EEPROM_OK) {
		return status;
	}
	if ((*(__io uint16*)PageBase0) == EEPROM_ERASED) {
		// Set Page0 as valid page: Write VALID_PAGE at Page0 base address
		FlashStatus = FLASH_ProgramHalfWord(PageBase0, EEPROM_VALID_PAGE);
		if (FlashStatus != FLASH_COMPLETE) {
			return FlashStatus;
		}
	}
	// Erase Page1
	return EE_CheckErasePage(PageBase1, EEPROM_ERASED);
}

/**
  * @brief  Returns the erase counter for current page
  * @param  Data: Global variable contains the read variable value
  * @retval Success or error status:
  *			- EEPROM_OK: if erases counter return.
  *			- EEPROM_NO_VALID_PAGE: if no valid page was found.
  */
uint16 IEEPROMClass::erases(uint16 *Erases)
{
	uint32 pageBase;
	if (Status != EEPROM_OK) {
		if (init() != EEPROM_OK) {
			return Status;
		}
	}

	// Get active Page for read operation
	pageBase = EE_FindValidPage();
	if (pageBase == 0) {
		return  EEPROM_NO_VALID_PAGE;
	}

	*Erases = (*(__io uint16*)pageBase + 2);
	return EEPROM_OK;
}

/**
  * @brief	Returns the last stored variable data, if found,
  *			which correspond to the passed virtual address
  * @param  Address: Variable virtual address
  * @retval Data for variable or EEPROM_DEFAULT_DATA, if any errors
  */
uint16 IEEPROMClass::read (uint16 Address)
{
	uint16 data;
	read(Address, &data);
	return data;
}

/**
  * @brief	Returns the last stored variable data, if found,
  *			which correspond to the passed virtual address
  * @param  Address: Variable virtual address
  * @param  Data: Pointer to data variable
  * @retval Success or error status:
  *           - EEPROM_OK: if variable was found
  *           - EEPROM_BAD_ADDRESS: if the variable was not found
  *           - EEPROM_NO_VALID_PAGE: if no valid page was found.
  */
uint16 IEEPROMClass::read(uint16 Address, uint16 *Data)
{
	uint32 pageBase, pageEnd;

	// Set default data (empty EEPROM)
	*Data = EEPROM_DEFAULT_DATA;

	if (Status == EEPROM_NOT_INIT) {
		if (init() != EEPROM_OK) {
			return Status;
		}
	}

	// Get active Page for read operation
	pageBase = EE_FindValidPage();
	if (pageBase == 0) {
		return  EEPROM_NO_VALID_PAGE;
	}

	// Get the valid Page end Address
	pageEnd = pageBase + ((uint32)(PageSize - 2));

	// Check each active page address starting from end
	for (pageBase += 6; pageEnd >= pageBase; pageEnd -= 4)
		if ((*(__io uint16*)pageEnd) == Address) {	// Compare the read address with the virtual address
			*Data = (*(__io uint16*)(pageEnd - 2));		// Get content of Address-2 which is variable value
			return EEPROM_OK;
		}

	// Return ReadStatus value: (0: variable exist, 1: variable doesn't exist)
	return EEPROM_BAD_ADDRESS;
}

/**
  * @brief  Writes/upadtes variable data in EEPROM.
  * @param  VirtAddress: Variable virtual address
  * @param  Data: 16 bit data to be written
  * @retval Success or error status:
  *			- FLASH_COMPLETE: on success
  *			- EEPROM_BAD_ADDRESS: if address = 0xFFFF
  *			- EEPROM_PAGE_FULL: if valid page is full
  *			- EEPROM_NO_VALID_PAGE: if no valid page was found
  *			- EEPROM_OUT_SIZE: if no empty EEPROM variables
  *			- Flash error code: on write Flash error
  */
uint16 IEEPROMClass::write(uint16 Address, uint16 Data)
{
	if (Status == EEPROM_NOT_INIT) {
		if (init() != EEPROM_OK) {
			return Status;
		}
	}

	if (Address == 0xFFFF) {
		return EEPROM_BAD_ADDRESS;
	}

	// Write the variable virtual address and value in the EEPROM
	uint16 status = EE_VerifyPageFullWriteVariable(Address, Data);
	return status;
}

/**
  * @brief  Return number of variable
  * @retval Number of variables
  */
uint16 IEEPROMClass::count(uint16 *Count)
{
	if (Status == EEPROM_NOT_INIT) {
		if (init() != EEPROM_OK) {
			return Status;
		}
	}

	// Get valid Page for write operation
	uint32 pageBase = EE_FindValidPage();
	if (pageBase == 0) {
		return EEPROM_NO_VALID_PAGE;	// No valid page, return max. numbers
	}

	*Count = EE_GetVariablesCount(pageBase, 0xFFFF);
	return EEPROM_OK;
}

uint16 IEEPROMClass::maxcount(void)
{
	return ((PageSize / 4) - 1);
}

/**
  * @brief  Inserts a time delay.
  * @param  None
  * @retval None
  */
static void delay(void)
{
	__io uint32 i = 0;
	for (i = 0xFF; i != 0; i--) { }
}

/**
  * @brief  Returns the FLASH Status.
  * @param  None
  * @retval FLASH Status: The returned value can be: FLASH_BUSY, FLASH_ERROR_PG,
  *   FLASH_ERROR_WRP or FLASH_COMPLETE
  */
FLASH_Status FLASH_GetStatus(void)
{
	if ((FLASH_BASE->SR & FLASH_SR_BSY) == FLASH_SR_BSY)
		return FLASH_BUSY;

	if ((FLASH_BASE->SR & FLASH_SR_PGERR) != 0)
		return FLASH_ERROR_PG;

	if ((FLASH_BASE->SR & FLASH_SR_WRPRTERR) != 0 )
		return FLASH_ERROR_WRP;

	if ((FLASH_BASE->SR & FLASH_OBR_OPTERR) != 0 )
		return FLASH_ERROR_OPT;

	return FLASH_COMPLETE;
}

/**
  * @brief  Waits for a Flash operation to complete or a TIMEOUT to occur.
  * @param  Timeout: FLASH progamming Timeout
  * @retval FLASH Status: The returned value can be: FLASH_ERROR_PG,
  *   FLASH_ERROR_WRP, FLASH_COMPLETE or FLASH_TIMEOUT.
  */
FLASH_Status FLASH_WaitForLastOperation(uint32 Timeout)
{
	FLASH_Status status;

	/* Check for the Flash Status */
	status = FLASH_GetStatus();
	/* Wait for a Flash operation to complete or a TIMEOUT to occur */
	while ((status == FLASH_BUSY) && (Timeout != 0x00)) {
		delay();
		status = FLASH_GetStatus();
		Timeout--;
	}
	if (Timeout == 0)
		status = FLASH_TIMEOUT;
	/* Return the operation status */
	return status;
}

/**
  * @brief  Erases a specified FLASH page.
  * @param  Page_Address: The page address to be erased.
  * @retval FLASH Status: The returned value can be: FLASH_BUSY, FLASH_ERROR_PG,
  *   FLASH_ERROR_WRP, FLASH_COMPLETE or FLASH_TIMEOUT.
  */
FLASH_Status FLASH_ErasePage(uint32 Page_Address)
{
	FLASH_Status status = FLASH_COMPLETE;
	/* Check the parameters */
	ASSERT(IS_FLASH_ADDRESS(Page_Address));
	/* Wait for last operation to be completed */
	status = FLASH_WaitForLastOperation(EraseTimeout);

	if (status == FLASH_COMPLETE) {
		/* if the previous operation is completed, proceed to erase the page */
		FLASH_BASE->CR |= FLASH_CR_PER;
		FLASH_BASE->AR = Page_Address;
		FLASH_BASE->CR |= FLASH_CR_STRT;

		/* Wait for last operation to be completed */
		status = FLASH_WaitForLastOperation(EraseTimeout);
		if (status != FLASH_TIMEOUT) {
			/* if the erase operation is completed, disable the PER Bit */
			FLASH_BASE->CR &= ~FLASH_CR_PER;
		}
		FLASH_BASE->SR = (FLASH_SR_EOP | FLASH_SR_PGERR | FLASH_SR_WRPRTERR);
	}
	/* Return the Erase Status */
	return status;
}

/**
  * @brief  Programs a half word at a specified address.
  * @param  Address: specifies the address to be programmed.
  * @param  Data: specifies the data to be programmed.
  * @retval FLASH Status: The returned value can be: FLASH_ERROR_PG,
  *   FLASH_ERROR_WRP, FLASH_COMPLETE or FLASH_TIMEOUT.
  */
FLASH_Status FLASH_ProgramHalfWord(uint32 Address, uint16 Data)
{
	FLASH_Status status = FLASH_BAD_ADDRESS;

	if (IS_FLASH_ADDRESS(Address)) {
		/* Wait for last operation to be completed */
		status = FLASH_WaitForLastOperation(ProgramTimeout);
		if (status == FLASH_COMPLETE) {
			/* if the previous operation is completed, proceed to program the new data */
			FLASH_BASE->CR |= FLASH_CR_PG;
			*(__io uint16*)Address = Data;
			/* Wait for last operation to be completed */
			status = FLASH_WaitForLastOperation(ProgramTimeout);
			if (status != FLASH_TIMEOUT) {
				/* if the program operation is completed, disable the PG Bit */
				FLASH_BASE->CR &= ~FLASH_CR_PG;
			}
			FLASH_BASE->SR = (FLASH_SR_EOP | FLASH_SR_PGERR | FLASH_SR_WRPRTERR);
		}
	}
	return status;
}

/**
  * @brief  Unlocks the FLASH Program Erase Controller.
  * @param  None
  * @retval None
  */
void FLASH_Unlock(void)
{
	/* Authorize the FPEC Access */
	FLASH_BASE->KEYR = FLASH_KEY1;
	FLASH_BASE->KEYR = FLASH_KEY2;
}

/**
  * @brief  Locks the FLASH Program Erase Controller.
  * @param  None
  * @retval None
  */
void FLASH_Lock(void)
{
	/* Set the Lock Bit to lock the FPEC and the FCR */
	FLASH_BASE->CR |= FLASH_CR_LOCK;
}

IEEPROMClass ieepromClass;
