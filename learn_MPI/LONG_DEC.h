#pragma once
class LONG_DEC
{
	/*
	ATTENTION : CANNOT represent negative number;

	*/
public:
	using BYTE = char;
	using INT64 = signed long long;

	LONG_DEC() = default;

	LONG_DEC(const LONG_DEC& from);
	// convert constructer from in-built integer
	LONG_DEC(INT64 from);
	// convert constructer form a cstring buffer
	LONG_DEC(const char* from);

	void operator = (const LONG_DEC& from);
	// emplace addition
	void operator += (LONG_DEC& operand);
	// emplace subtraction
	void operator -= (LONG_DEC& operand);
	// emplace multiplication with a in-built integer
	INT64 int_mul(INT64 operand);
	// emplace devision with a in-built integer
	void int_div(INT64 operand);

	int operator == (const LONG_DEC& operand);
	
	void print(int precision=16);

	BYTE operator [](int index);

	char* _buff() { return _data; }

public:
	static const int INT_PART_SIZE = 20;
	static const int FRAC_PART_SIZE = 2000;
	static const int ARR_SIZE = INT_PART_SIZE + FRAC_PART_SIZE;
	static const int UNIT_DIG = INT_PART_SIZE - 1;


private:
	BYTE _data[ARR_SIZE]{};
};

