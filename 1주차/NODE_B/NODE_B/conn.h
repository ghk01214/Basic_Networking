#pragma once

class CONN
{
public:
	CONN();
	void set(bool value);
	void set_value(bool value);
	bool get();

private:
	bool stat;
};

extern CONN g_conn;