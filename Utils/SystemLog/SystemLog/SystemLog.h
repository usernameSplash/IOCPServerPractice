#pragma once

enum class eLogLevel
{
	DEBUG = 0,
	ERROR = 1,
	SYSTEM = 2
};

class SystemLog
{

private:
	eLogLevel _logLevel = eLogLevel::DEBUG;

};