// created by M.Świdziński, 10/2021
// last mod. 10.05.2022, build 26
// serial_rw.c - This program sends and read text via serial port
// Parameters:
// - /baudrate [value] - serial port speed
// - /comport [value] - serial port number
// - /closedelay [value] - time in ms to close port after send command
// - /rts_en [value] - RTS is HIGH for "value" time and after that is LOW (use to HW reset UD)
// - /dtr_off - disable DTR
// - /cctalk_cut - cut own sended message, print only anwser (echo disable)
// - /parity_even - use PARITY EVEN (default:NONE) used for bill dispenser Fujitsu
// - /help - usage help
// - /hex - string or hex to send to serial port optional end char:
//    - "\r" carrige return
//    - "\n" new line
//    - "\0" null char
//    - "\xBYTE" for send byte or "xBYTE1\xBYTE2\xBYTE3" to send byte array
// Auto parshing is implemented, if send message as string /hex "hello world\n", anwser will be
// printed as ASCII code. But if send byte or byte array e.g /hex "\xFF\x01\xC0\" anwser will be
// pinted as raw hex byte.
//
// example:
// read: serial_rw.exe /comport 2 /baudrate 115200 /closedelay 500
// write: serial_rw.exe /comport 2 /baudrate 57600 /closedelay 500 /dtr_off /hex "ExampleString\r\n"
//

#include <windows.h>
#include <stdio.h>
#include <time.h>

#define MESSAGE_LENGTH 100

int main(int argc, char *argv[])
{
    // Declare variables and structures
    int cctalkCutMode = 0;
    int sendBytes = 0;
    int bytesMode = 0;
    int parityeven = 0;
    int cnt = 0;
    int m, n;
    unsigned char buffer[MAX_PATH];
    unsigned char text_to_send[MAX_PATH];
    unsigned char digits[MAX_PATH];
    int baudrate = 9600;
    int dev_num = 50;
    int parse_hex_bytes = 0;
    int close_delay = 0;
	int rts_en = 0;
	int dtrEnable = 1;
    char dev_name[MAX_PATH];
    HANDLE hSerial;
    DCB dcbSerialParams = {0};
    COMMTIMEOUTS timeouts = {0};
	int id = -1;
	unsigned long timeStart;

    // Parse command line arguments
    int argn = 1;
    strcpy(buffer, "");
    while(argn < argc)
    {
        if (strcmp(argv[argn], "/baudrate") == 0)
        {
            // Parse baud rate
            if (++argn < argc && ((baudrate = atoi(argv[argn])) > 0))
            {
                fprintf(stdout, "%d baud specified\n", baudrate);
            }
            else
            {
                fprintf(stdout, "Baud rate error\n");
                return 1;
            }
        }
        else if (strcmp(argv[argn], "/comport") == 0)
        {
            // Parse device number. SerialSend actually just
            // begins searching at this number and continues
            // working down to zero.
            if (++argn < argc)
            {
                dev_num = atoi(argv[argn]);
            }
            else
            {
                fprintf(stdout, "Device number error\n");
                return 1;
            }
        }
        else if (strcmp(argv[argn], "/closedelay") == 0)
        {
            // Parse close delay duration. After transmitting
            // the specified text, SerialSend will delay by
            // this number of milliseconds before closing the
            // COM port. Some devices seem to require this.
            if (++argn < argc)
            {
                close_delay = atoi(argv[argn]);
                fprintf(stdout, "Delay of %d ms specified before closing COM port\n", close_delay);
            }
            else
            {
                fprintf(stdout, "Close delay error\n");
                return 1;
            }
		}
		else if (strcmp(argv[argn], "/rts_en") == 0)
		{
            // Enable RTS
            if (++argn < argc)
            {
                rts_en = atoi(argv[argn]);
                fprintf(stdout, "RTS is enable\n");
            }
            else
            {
                fprintf(stdout, "Enable RTS error\n");
                return 1;
            }
        }
		else if (strcmp(argv[argn], "/dtr_off") == 0)
        {
            dtrEnable = 0;
        }
        else if (strcmp(argv[argn], "/parity_even") == 0)
        {
            parityeven = 1;
        }
        else if (strcmp(argv[argn], "/cctalk_cut") == 0)
        {
            cctalkCutMode = 1;
        }
        else if (strcmp(argv[argn], "/help") == 0)
        {
            fprintf(stdout, "serial_rw v1.0 - write and read from serial port\n");
            fprintf(stdout, "Usage:\nserial_rw.exe [/baudrate BAUDRATE] ");
            fprintf(stdout, "[/comport NUMBER] [/hex \"COMMAND\"]\n");
            fprintf(stdout, "[/closedelay TIME_MS] [/rts_en TIME_MS] [/dtr_off]\n");
            fprintf(stdout, "[/cctalk_cut] - cut own byte, print only anwser.\n");
            fprintf(stdout, "[/parity_even] - use PARITY_EVEN, def. NONE.\n");
            return 1;
        }
        else if (strcmp(argv[argn], "/hex") == 0)
        {
            // Parse flag for hex byte parsing.
            // If this flag is set, then arbitrary byte values can be
            // included in the string to send using '\x' notation.
            // For example, the command "serial_rw.exe /hex Hello\x0D"
            // sends six bytes in total, the last being the carriage
            // return character, '\r' which has hex value 0x0D.
            parse_hex_bytes = 1;
        }
        else
        {
            // This command line argument is the text to send
            strcpy(buffer, argv[argn]);
        }

        // Next command line argument
        argn++;
    }


    // If hex parsing is enabled, modify text to send
    n = 0; m = 0;
    while(n < strlen(buffer))
    {
        if (parse_hex_bytes && buffer[n] == '\\')
        {
            n++;
            if (buffer[n] == '\\') text_to_send[m] = '\\';
            else if (buffer[n] == 'n') text_to_send[m] = '\n';
            else if (buffer[n] == 'r') text_to_send[m] = '\r';
            else if (buffer[n] == 'x')
            {
                bytesMode=1;
                digits[0] = buffer[++n];
                digits[1] = buffer[++n];
                digits[2] = '\0';
                text_to_send[m] = strtol(digits, NULL, 16);
            }
        }
        else
        {
            text_to_send[m] = buffer[n];
        }

        m++; n++;
    }
    text_to_send[m] = '\0'; // Null character to terminate string

    // Open the highest available serial port number
    if (dev_num > 0)
    {
        sprintf(dev_name, "\\\\.\\COM%d", dev_num);
        hSerial = CreateFile(
        dev_name, GENERIC_READ|GENERIC_WRITE, 0, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
        if (hSerial == INVALID_HANDLE_VALUE) dev_num--;
        //else break;
    }


    if (dev_num < 0)
    {
        fprintf(stdout, "ERROR: No serial port set!\n");
        return 1;
    }

    // Set device parameters (38400 baud, 1 start bit,
    // 1 stop bit, no parity)
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (GetCommState(hSerial, &dcbSerialParams) == 0)
    {
        fprintf(stdout, "ERROR: No serial port available!\n");
        CloseHandle(hSerial);
        return 1;
    }

    fprintf(stdout, "Open serial port...OK\n");

    //number od sended byte in messegage - dec value
    sendBytes=n/4;
    //show number preview message
    //printf("%d", sendBytes);

    //dcbSerialParams.BaudRate = CBR_38400;
    dcbSerialParams.BaudRate = baudrate;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    if (parityeven == 1)
	{
        dcbSerialParams.Parity = EVENPARITY;
        fprintf(stdout, "Parity EVEN active.\n");
	}
	else {
        dcbSerialParams.Parity = NOPARITY;
	}
    if(SetCommState(hSerial, &dcbSerialParams) == 0)
    {
        fprintf(stdout, "Error setting device parameters\n");
        CloseHandle(hSerial);
        return 1;
    }

    // Set COM port timeout settings
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;
    if(SetCommTimeouts(hSerial, &timeouts) == 0)
    {
        fprintf(stdout, "Error setting timeouts\n");
        CloseHandle(hSerial);
        return 1;
    }

	//Put HIGH RTS line for 5 second and then put LOW it is use to drive N-mosfet to hardware reset UD 24V supply.
    if (rts_en > 0)
	{
		EscapeCommFunction(hSerial, SETRTS);
		fprintf(stdout, "RTS active for %d ms", rts_en);
		fprintf(stdout, "\n");
		Sleep(rts_en);
		//EscapeCommFunction(hSerial, CLRRTS);
	}

	//Disable DTR when running with parameter: /dtr_off
	if (dtrEnable == 1)
	{
        EscapeCommFunction(hSerial, SETDTR );
        fprintf(stdout, "DTR is enable.\n");
	}
	if (cctalkCutMode == 1)
	{
        fprintf(stdout, "ccTalk cut echo enable.\n");
	}

    //Sleep(1000);
    // Send specified text
    DWORD bytes_written, total_bytes_written = 0;
    while(total_bytes_written < m)
    {
        if(!WriteFile(hSerial, text_to_send + total_bytes_written,
            m - total_bytes_written, &bytes_written, NULL))
        {
            fprintf(stdout, "Error writing text to %s\n", dev_name);
            CloseHandle(hSerial);
            return 1;
        }

        total_bytes_written += bytes_written;
    }
    Sleep(10);
	// Read text and print to console (and maybe simulate keystrokes)
    int state = 1;

    //for byte message
    byte b;
    //for string message
    char c;
    char message_buffer[MESSAGE_LENGTH];
    DWORD bytes_read;

    // Depending on whether a robot id has been specified, either
    // print all incoming characters to the console or filter by
    // the specified id number
    if (id == -1)
    {
        // No robot id specified - print everything to console
		timeStart = clock();
        while(1)
        {
            cnt = cnt + 1;
            //printf("%d", cnt);
            if (bytesMode==1) {
                ReadFile(hSerial, &b, 1, &bytes_read, NULL);
            }
            else {
                ReadFile(hSerial, &c, 1, &bytes_read, NULL);
            }

            if (bytes_read == 1) {
            //print raw hex bytes is message will be send as byte \x00\xFF
                if (bytesMode==1) {
                    //filter echo - print only anwser not print own sent message
                    if (cctalkCutMode == 1) {
                        if (cnt > sendBytes)  {
                            printf("%02X", b);
                        }
                    }

                    else {
                        printf("%02X", b);
                    }
                }
                //prinf ascii char is message will send as string
                else {
                    printf("%c", c);
                }

            } else {

				if (close_delay >0)
				{
					// time in milliseconds to exit after read data
					if ((clock() - timeStart) / (CLOCKS_PER_SEC/1000) >= (close_delay))
					{
						// Flush transmit buffer before closing serial port
						FlushFileBuffers(hSerial);
						printf("\n");
						break;
					}

				} else
				{
				    printf("\n");
					break;
				}

			}
        }
    }
	EscapeCommFunction(hSerial, CLRRTS);
	EscapeCommFunction(hSerial, CLRDTR);

    // Close serial port
    if (CloseHandle(hSerial) == 0)
    {
        fprintf(stdout, "Error", dev_name);
        return 1;
    }

    // exit normally
    return 0;
}
