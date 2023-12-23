#include "LONG_DEC.h"

#include <cstdio>
#include <cstring>

LONG_DEC::LONG_DEC(const LONG_DEC& from) {
	for (int i = 0; i < ARR_SIZE; ++i) {
		this->_data[i] = from._data[i];
	}
}

// convert constructer from in-built integer
LONG_DEC::LONG_DEC(INT64 from){
	//memset(this->_data, 0, ARR_SIZE);
	int i = UNIT_DIG;
	while (from != 0 && i >= 0) {
		this->_data[i] = static_cast<BYTE>(from % 10);
		--i;
		from /= 10;
	}
}

// convert constructer form a cstring buffer
LONG_DEC::LONG_DEC(const char* from){
	// The user has obligation to ensure the invalidity of the string buffer;
	int i = 0, j, k;
	// find the fraction point;
	while (i < ARR_SIZE && from[i] != '.') ++i;
	j = i - 1;
	k = UNIT_DIG;
	while (k >= 0 && j >= 0) this->_data[k--] = from[j--] - '0';
	// The integer part in the buffer that is larger than INT_PART_SIZE will be ignored; 
	k = UNIT_DIG + 1;
	j = i + 1;
	while (from[j] && k < ARR_SIZE) {
		this->_data[k++] = from[j++] - '0';
	}
}

void LONG_DEC::operator = (const LONG_DEC& from) {
	for (int i = 0; i < ARR_SIZE; ++i) {
		this->_data[i] = from._data[i];
	}
}

// emplace addition
void LONG_DEC::operator += (LONG_DEC& operand) {
	int i;
	BYTE carry = 0, tmp = 0;
	for (i = ARR_SIZE - 1; i >= 0; --i) {
		tmp = this->_data[i] + operand._data[i] + carry;
		this->_data[i] = tmp % 10;
		carry = tmp > 9 ? 1 : 0;
	}
	return;
}

// emplace subtraction
void LONG_DEC::operator -= (LONG_DEC& operand) {
	int i = ARR_SIZE - 1;
	BYTE borrow = 0, tmp = 0;
	for (i = ARR_SIZE - 1; i >= 0; --i) {
		tmp = this->_data[i] - borrow - operand._data[i];
		if (tmp < 0) {
			tmp += 10;
			borrow = 1;
		}
		else borrow = 0;
		this->_data[i] = tmp;
	}
	return;
}

// emplace multiplication with a in-built integer
LONG_DEC::INT64 LONG_DEC::int_mul(INT64 operand) {
	// return carry;
	INT64 carry = 0;
	INT64 tmp = 0;
	for (int i = ARR_SIZE - 1; i >= 0; --i) {
		tmp = this->_data[i] * operand + carry;
		carry = tmp / 10;
		this->_data[i] = tmp % 10;
	}
	return carry;
}

// emplace devision with a in-built integer
void LONG_DEC::int_div(INT64 operand) {
	INT64 rem = 0;
	INT64 tmp = 0;
	for (int i = 0; i < ARR_SIZE; ++i) {
		tmp = rem * 10 + this->_data[i];
		//if (tmp = 0 && rem == 0) break;
		this->_data[i] = static_cast<BYTE>(tmp / operand);
		rem = tmp % operand;
	}
	return;
}

int LONG_DEC::operator == (const LONG_DEC& operand){
	int cnt = 0;
	while (cnt < ARR_SIZE) {
		if (this->_data[cnt] != operand._data[cnt])
			return cnt;
		++cnt;
	}
	return cnt;
}


void LONG_DEC::print(int precision) {
	precision += UNIT_DIG;
	if (precision > ARR_SIZE) precision = ARR_SIZE;
	int i = 0, j = ARR_SIZE - 1;
	while (j >= 0 && this->_data[j] == 0) --j;
	if (j < 0) {
		printf("0");
		return;
	}
	while (i <= UNIT_DIG && this->_data[i] == 0) ++i;
	if (i > UNIT_DIG) printf("0.");
	else {
		while (i <= UNIT_DIG) {
			printf("%hhu", this->_data[i]);
			++i;
		}
		printf(".");
		while (i <= precision) {
			printf("%hhu", this->_data[i]);
			++i;
		}
	}
	return;
}

LONG_DEC::BYTE LONG_DEC::operator [](int index) {
	return this->_data[index];
}
