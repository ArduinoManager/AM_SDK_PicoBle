/*
   Arduino Manager for iPad / iPhone / Mac

   A simple skeleton program to adapt to work with Arduino Manager

   Author: Fabrizio Boco - fabboco@gmail.com

   Version: 1.0

   12/16/2024

   All rights reserved
*/

/*
   AMController libraries, example sketches (The Software) and the related documentation (The Documentation) are supplied to you
   by the Author in consideration of your agreement to the following terms, and your use or installation of The Software and the use of The Documentation
   constitutes acceptance of these terms.
   If you do not agree with these terms, please do not use or install The Software.
   The Author grants you a personal, non-exclusive license, under authors copyrights in this original software, to use The Software.
   Except as expressly stated in this notice, no other rights or licenses, express or implied, are granted by the Author, including but not limited to any
   patent rights that may be infringed by your derivative works or by other works in which The Software may be incorporated.
   The Software and the Documentation are provided by the Author on an AS IS basis.  THE AUTHOR MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT
   LIMITATION THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, REGARDING THE SOFTWARE OR ITS USE AND OPERATION
   ALONE OR IN COMBINATION WITH YOUR PRODUCTS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR CONSEQUENTIAL DAMAGES (INCLUDING,
   BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE,
   REPRODUCTION AND MODIFICATION OF THE SOFTWARE AND OR OF THE DOCUMENTATION, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE),
   STRICT LIABILITY OR OTHERWISE, EVEN IF THE AUTHOR HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"

#include "AM_SDK_PicoBle.h"

/* Defines */

/* Gobal variables */

AMController am_controller;

/* Local Function Prototypes */

/* Callbacks */

/** 
 * 
 * This function is called when the iOS/macOS device connects to the Pico board
 * 
 */
void deviceConnected()
{
    printf("---- deviceConnected --------\n");
}

/** 
 * 
 * This function is called when the iOS/macOS device disconnects to the Pico board
 * 
 */
void deviceDisconnected()
{
    printf("---- deviceDisonnected --------\n");
}

/**
 *
 *
 * This function is called when the iOS/macOS device connects and needs to initialize the position of switches, knobs and other widgets
 *
 */
void doSync()
{
    printf("---- doSync --------\n");
}

/**
 *
 *
 * This function is called periodically and its equivalent to the standard loop() function
 *
 */
void doWork()
{
    // printf("doWork\n");

    // sleep_ms(100);
}

/**
 *
 *
 * This function is called when a new message is received from the iOS/macOS device
 *
 */
void processIncomingMessages(char *variable, char *value)
{
    // printf("processIncomingMessages var: %s value: %s\n", variable, value);
}

/**
 *
 *
 * This function is called periodically and messages can be sent to the iOS/macOS device
 *
 */
void processOutgoingMessages()
{
    // printf("processOutgoingMessages\n");
}

/**
 *
 *
 * This function is called when an alarm is fired
 *
 */
void processAlarms(char *alarmId)
{
    printf("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
    printf("Alarm %s fired\n", alarmId);
    printf("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
}

/**
  Other Auxiliary functions
*/


/**
 * 
 * 
 * Main program function used for initial configurations only
 * 
 * 
 */
int main()
{
    stdio_init_all();

    if (cyw43_arch_init())
    {
        printf("Failed to initialize\n");
        return 1;
    }

    /** Initialize analog and digital PINs */

    am_controller.init(
        &doWork,
        &doSync,
        &processIncomingMessages,
        &processOutgoingMessages,
        &deviceConnected,
        &deviceDisconnected,
        &processAlarms);

    cyw43_arch_deinit();
}
