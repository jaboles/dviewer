#include "types.h"
#include "PCI_LIB.h"
#include <dos.h>

int pcibios_present(void)
{
   int ret_status;          /* Function Return Status. */
   byte bios_present_status;/* Indicates if PCI bios present */
   dword pci_signature;     /* PCI Signature ('P', 'C', 'I', ' ') */
   word ax, cx, flags;  /* Temporary variables to hold register values */


   /* Load entry registers for PCI BIOS */
   _AH = PCI_FUNCTION_ID;
   _AL = PCI_BIOS_PRESENT;

   /* Call PCI BIOS Int 1Ah interface */
   geninterrupt(0x1a);

   /* Save registers before overwritten by compiler usage of registers */
   ax = _AX;
   cx = _CX;
   pci_signature = _EDX;
   flags = _FLAGS;
   bios_present_status = HIGH_BYTE(ax);


   /* First check if CARRY FLAG Set, if so, BIOS not present */
   if ((flags & CARRY_FLAG) == 0) {

      /* Next, must check that AH (BIOS Present Status) == 0 */
      if (bios_present_status == 0) {

	 /* Check bytes in pci_signature for PCI Signature */
	 if ((pci_signature & 0xff)         == 'P' &&
	     ((pci_signature >> 8) & 0xff)  == 'C' &&
	     ((pci_signature >> 16) & 0xff) == 'I' &&
	     ((pci_signature >> 24) & 0xff) == ' ') {

	    /* Extract calling parameters from saved registers */
		ret_status = LOW_BYTE(cx);
	 }
      }
      else {
	 ret_status = -1;
      }
   }
   else {
      ret_status = -1;
   }

   return (ret_status);
}

static int read_configuration_area(byte function,
				   byte bus_number,
				   byte device_and_function,
				   byte register_number,
				   dword *data)
{
   int ret_status; /* Function Return Status */
   word ax, flags; /* Temporary variables to hold register values */
   dword ecx;      /* Temporary variable to hold ECX register value */

   /* Load entry registers for PCI BIOS */
   _BH = bus_number;
   _BL = device_and_function;
   _DI = register_number;
   _AH = PCI_FUNCTION_ID;
   _AL = function;

   /* Call PCI BIOS Int 1Ah interface */
   geninterrupt(0x1a);

   /* Save registers before overwritten by compiler usage of registers */
   ecx = _ECX;
   ax = _AX;
   flags = _FLAGS;

   /* First check if CARRY FLAG Set, if so, error has occurred */
   if ((flags & CARRY_FLAG) == 0) {

      /* Get Return code from BIOS */
      ret_status = HIGH_BYTE(ax);

      /* If successful, return data */
      if (ret_status == SUCCESSFUL) {
	 *data = ecx;
      }
   }
   else {
      ret_status = NOT_SUCCESSFUL;
   }

   return (ret_status);
}


int read_configuration_byte(byte bus_number,
				 byte device_and_function,
				 byte register_number,
				 byte *byte_read)
{
	int ret_status; /* Function Return Status */
	dword data;

	/* Call read_configuration_area function with byte data */
	ret_status = read_configuration_area(READ_CONFIG_BYTE,
					bus_number,
					device_and_function,
					register_number,
					&data);
	if (ret_status == SUCCESSFUL) {

		/* Extract byte */
		*byte_read = (byte)(data & 0xff);
	}

	return (ret_status);
}
