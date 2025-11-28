*** Settings ***
Library  String
Library  SerialLibrary

*** Variables ***
${com}				COM5
${baud}				115200
${board}			nRF
${valid_time}		000120X
${invalid_time}		000099X
${too_long}			0011111X
${too_short}		00001X
${zero}				000000X
${seconds}			80
${value_error}		-3
${lenght_error}		-1

*** Test Cases ***
Connect Serial
	Log To Console  Connecting to  ${board}
	Add Port  ${com}  baudrate=${baud}  encoding=ascii
	Port Should Be Open  ${com}
	Reset Input Buffer
	Reset Output Buffer

Test Valid Time
	Reset Input Buffer
	Reset Output Buffer

	Write Data  ${valid_time}  encoding=ascii
	Log To Console  Send valid time ${valid_time}

	${read} =  Read Until  terminator=88  encoding=ascii
	${cleaned} =  Set Variable  ${read[:-1]}
	Log To Console  Received ${cleaned}

	Should Be Equal As Strings  ${cleaned}  ${seconds}
	Log To Console  Tested ${cleaned} is same as ${seconds}

Test Invalid Time
	Reset Input Buffer
	Reset Output Buffer

	Write Data  ${invalid_time}  encoding=ascii
	Log To Console  Send invalid time ${invalid_time}

	${read} =  Read Until  terminator=88  encoding=ascii
	${cleaned} =  Set Variable  ${read[:-1]}
	Log To Console  Received ${cleaned}

	Should Be Equal As Strings  ${cleaned}  ${value_error}
	Log To Console  Tested ${cleaned} is same as ${value_error}

Test String Length Under
	Reset Input Buffer
	Reset Output Buffer

	Write Data  ${too_short}  encoding=ascii
	Log To Console  Send too short string ${too_short}

	${read} =  Read Until  terminator=88  encoding=ascii
	${cleaned} =  Set Variable  ${read[:-1]}
	Log To Console  Received ${cleaned}

	Should Be Equal As Strings  ${cleaned}  ${lenght_error}
	Log To Console  Tested ${cleaned} is the same as ${lenght_error}

Test String Length Over
	Reset Input Buffer
	Reset Output Buffer

	Write Data  ${too_long}  encoding=ascii
	Log To Console  Send too long string ${too_long}

	${read} =  Read Until  terminator=88  encoding=ascii
	${cleaned} =  Set Variable  ${read[:-1]}
	Log To Console  Received ${cleaned}

	Should Be Equal As Strings  ${cleaned}  ${lenght_error}
	Log To Console  Tested ${cleaned} is the same as ${lenght_error}

Test Zero Seconds
	Reset Input Buffer
	Reset Output Buffer

	Write Data  ${zero}  encoding=ascii
	Log To Console  Send zero seconds ${zero}

	${read} =  Read Until  terminator=88  encoding=ascii
	${cleaned} =  Set Variable  ${read[:-1]}
	Log To Console  Received ${cleaned}

	Should Be Equal As Strings  ${cleaned}  ${value_error}
	Log To Console  Tested ${cleaned} is the same as ${value_error}

Disconnect Serial
	Log To Console  Disconnecting ${board}
	[TearDown]  Delete Port  ${com}
