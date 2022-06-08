@rem testowy drukarki custom vkp80ii, pomiety jest tu proces incjalizacji ustawiani tesktu itp. 
@echo off
:start
@rem wartosc poczatkow licznika aktualnie drukowych biletow 
set /a counter=0
echo *** DRUKOWANIE BILETOW ***
echo.
set /p ticketNo="Wpisz liczbe biletow: "
@rem liczba biletow do wydrukowania
set /a ticketNoDec=%ticketNo%
echo.
@rem jezeli liczba biletow wieksza niz 9, wypisze info o max. 9 znakach i wroci do poczatku
@rem 9 znakow max. bo 1 ascii hex to 31 a 39 to 9 zeby bylo porosciej, przy wiekszych liczbach trzeba dodac konwerter hex ascii a to jest skrypt tylko testowy...
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
@rem gdy wydrukuje wszystkie znaki przechodzi do podsumowania - info koncowego
if %counter% equ %ticketNoDec% goto done
@rem licznik aktualnie drukowanego biletu, zwieksza się z kazda petla 
set /a counter=%counter%+1
@rem wyswietla status wydruku, który bilet drukuje z wpisanej ilosci 
echo Trwa drukowanie biletu %counter% z %ticketNo%
@rem odwolanie do apki serial_rw.exe napisanej w C do komunikacji z portem szeregowym, parametry transmisji i dane ustawiane parametrami jak nizej
@rem wysyła na port szereowy COM2 teskst do drukarki: BILET i wpisaną liczbę od 1-9 + 3x znak LF, jeden do zakończenia przejścia wiersza,  dwa dla gotowości do cięcia papieru
for /F "delims=" %%A in ('serial_rw.exe /comport 2 /baudrate 19200 /dtr_off /closedelay 2000 /hex "\x42\x49\x4C\x45\x54\x20\x3%counter%\x0A\x0A\x0A"') do set anwser=%%A
@rem opoznienie przed cięciem papieru
timeout 1 >nul
@rem cięcie całkowite paieru 
for /F "delims=" %%A in ('serial_rw.exe /comport 2 /baudrate 19200 /dtr_off /closedelay 1000 /hex "\x1B\x69"') do set anwser=%%A
@rem opoźnienie przez podanie biletu
timeout 1 >nul
@rem wydaje bilet ale przytrzymuje go (nie wyrzuca)
for /F "delims=" %%A in ('serial_rw.exe /comport 2 /baudrate 19200 /dtr_off /closedelay 1000 /hex "\x1D\x65\x03\x0C"') do set anwser=%%A
echo.
echo Odbierz bilet!
@rem opoznienie przed sprawdzeniem stanu podajnika biletow
timeout 3 >nul
:WaitForRemoveTicket
@rem sprawdza stan podajnika, w odpowiedzi to 3 bajt (cale info w datasheet drukarki), w skrócie  0x44 (brak biletu w podajniku), 0x64 podajnik zajęty - do tak w duzym skrócie bo ten bajt trzeba
@rem sobie do skonwertować do postaci binarnej i tam na innych bitach są inne statusy, tu zrobione na pałe sugerujac sie defaultowymi usawieniami  
for /F "delims=" %%A in ('serial_rw.exe /comport 2 /baudrate 19200 /dtr_off /closedelay 2000 /hex "\x10\x04\x14"') do set anwser=%%A
@rem jezeli podajnik biletow jest pusty to przechodzi do kolejnego wydruku
if "%anwser%"=="100F44008000" goto PrintTicket
@rem jezli podajnik zajety, tj. poprzedni bilet nie zostal wyciahgniety, pokaze info aby odebrc najpierw poprzed bilet, po 5 sekunach sprawdza ponownie i tak w kolko poki podajnik sie nie zwoli 
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