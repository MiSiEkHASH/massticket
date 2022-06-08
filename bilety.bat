@rem testowy drukarki custom vkp80ii, pominiety jest tu proces incjalizacji, ustawiania tesktu, druku grafiki itp.
@rem port szeregowy wpisywany z palca 
@echo off

:start
@rem numer portu szeregowego, zamienic wartosc na uzywana, akceptowane wartosci 1-255 dec
set /a serialport=2
@rem wartosc poczatkowa licznika aktualnie drukowych biletow 
set /a counter=0
echo *** DRUKOWANIE BILETOW ***
echo.
set /p ticketNo="Wpisz liczbe biletow: "
@rem liczba biletow do wydrukowania
set /a ticketNoDec=%ticketNo%
echo.
@rem jezeli liczba biletow wieksza niz 9, wypisze info o max. 9 znakach i wroci do poczatku
@rem 9 znakow max. bo 1 ascii hex to 31 a 39 to 9 zeby bylo prosciej, podstawiam jedna cyfre, przy wiekszych liczbach trzeba by dodac konwerter hex ascii a to jest skrypt tylko testowy...
if %ticketNoDec% gtr 9 (
	echo Max. ilosc to 9 sztuk, wybierz mniejsza ilosc!
	timeout 4 > nul
	cls
	goto start
)
echo Wybrano drukowanie: %ticketNoDec% biletow, prosze czekac...
timeout 3 >nul
echo.
echo.

:PrintTicket
cls
@rem gdy wydrukuje wszystkie znaki przechodzi do podsumowania - info koncowego, jesli nie pomija
if %counter% equ %ticketNoDec% goto done
@rem licznik aktualnie drukowanego biletu, zwieksza się z kazda petla 
set /a counter=%counter%+1
@rem wyswietla status wydruku, który bilet drukuje z wpisanej ilosci 
echo Trwa drukowanie biletu %counter% z %ticketNo%
@rem odwolanie do apki serial_rw.exe napisanej w C do komunikacji z portem szeregowym, parametry transmisji i dane ustawiane parametrami jak nizej
@rem wysyła na port szereowy teskst (hex ascii) do drukarki: "BILET" i wpisaną liczbę od 1-9 + 3x znak LF(0x0A), jeden do zakonczenia przejscia wiersza,  dwa dla gotowosci do ciecia papieru
for /F "delims=" %%A in ('serial_rw.exe /comport %serialport% /baudrate 19200 /dtr_off /closedelay 2000 /hex "\x42\x49\x4C\x45\x54\x20\x3%counter%\x0A\x0A\x0A"') do set anwser=%%A
@rem opoznienie przed cieciem papieru
timeout 1 >nul
@rem ciecie calkowite papieru 0x1B, 0x69
for /F "delims=" %%A in ('serial_rw.exe /comport %serialport% /baudrate 19200 /dtr_off /closedelay 1000 /hex "\x1B\x69"') do set anwser=%%A
@rem opoznienie przed podaniem biletu
timeout 1 >nul
@rem wydaje bilet ale przytrzymuje go (nie wyrzuca) 0x1D, 0x65, 0x03, 0x0C
for /F "delims=" %%A in ('serial_rw.exe /comport %serialport% /baudrate 19200 /dtr_off /closedelay 1000 /hex "\x1D\x65\x03\x0C"') do set anwser=%%A
echo.
echo Odbierz bilet!
@rem opoznienie przed sprawdzeniem stanu podajnika biletow
timeout 3 >nul

:WaitForRemoveTicket
@rem sprawdza stan podajnika, w odpowiedzi to 3 bajt z 6 (cale info w datasheet drukarki), w skrocie 0x44 (brak biletu w podajniku), 0x64 podajnik zajety - do tak w duzym skrocie bo ten bajt trzeba
@rem sobie skonwertowac do postaci binarnej i tam na innych bitach są inne statusy, tu zrobione na pałe sugerujac sie defaultowymi ustawieniami  
for /F "delims=" %%A in ('serial_rw.exe /comport %serialport% /baudrate 19200 /dtr_off /closedelay 2000 /hex "\x10\x04\x14"') do set anwser=%%A
@rem jezeli podajnik biletow jest pusty to przechodzi do kolejnego wydruku
if "%anwser%"=="100F44008000" goto PrintTicket
@rem jezli podajnik zajety, tj. poprzedni bilet nie zostal wyciagniety, pokaze info aby odebrc najpierw poprzedni bilet, po 5 sekunach sprawdza ponownie i tak w kolko poki podajnik sie nie zwoli 
if "%anwser%"=="100F64008000" (
	cls
	echo.
	echo Najpierw odbierz poprzedni bilet
	echo.
	timeout 5 >nul
	goto WaitForRemoveTicket
)
pause
echo.

:done
cls
echo.
echo Dziekujemy za wydruk wszystkich %ticketNoDec% biletow!
echo.
pause