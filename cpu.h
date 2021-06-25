#ifndef CPU6502_H
#define CPU6502_H

#include <sstream>
#include <fstream>
#include <cstdint>
#include <string>
#include <array>
#include <vector>
#include "SDL.h"

#define CARRY  0x1
#define ZERO   0x2
#define INTR   0x4
#define DEC    0x8
#define BRK    0x10
#define _      0x20
#define OVRFLW 0x40
#define NEG	   0x80

#define BASESTACK 0x100

#define NEG_RES (status & NEG)? 1 : 0
#define CAR_RES (status & CARRY)? 1 : 0
#define ZER_RES (status & ZERO)? 1 : 0
#define DEC_RES (status & DEC)? 1 : 0
#define BRK_RES (status & BRK)? 1 : 0
#define OVR_RES (status & OVRFLW)? 1 : 0
#define INT_RES (status & INTR)? 1 : 0
#define _RES    (status & _)? 1 : 0

class Bus;

class cpu6502
{
public:
	uint8_t A;
	uint8_t X, Y;
	uint8_t sp;
	uint8_t status;
	uint16_t pc;
	Bus *busPtr;
	uint8_t ram[0xffff];
public:

	//
	void ConnectTotBus(Bus* ptr) {
		busPtr = ptr;
	}

	//debugging and logging
	int cycleCount;
	std::string addr_string = "No Address Availible";
	std::string op_string = "No Instruction Executed";
	uint16_t getPC() { return pc; }
	void setPC(uint16_t pc_) { pc = pc_; }
	void printIns() { 
		printf(std::string(op_string).c_str());
		printf(" ");
		printf(std::string(addr_string).c_str());
		printf("\n");
	}

	//Bus read and write, and load functions
	void write(uint16_t addr, uint8_t data);
	uint16_t read(uint16_t addr);

	//status flag operator
	void set_neg(bool s) { s ? status |= NEG : status &= (~NEG); }
	void set_ovr(bool s) { s ? status |= OVRFLW : status &= (~OVRFLW); }
	void set_car(bool s) { s ? status |= CARRY : status &= (~CARRY); }
	void set_brk(bool s) { s ? status |= BRK : status &= (~BRK); }
	void set_zer(bool s) { s ? status |= ZERO : status &= (~ZERO); }
	void set_int(bool s) { s ? status |= INTR : status &= (~INTR); }
	void set_dec(bool s) { s ? status |= DEC : status &= (~DEC); }
	void set_unused(bool s) { s ? status |= _ : status &= (~_); }

	//stack
	void pushtostack(uint8_t);
	uint8_t pullfromstack();

	//Cycle functions
	bool zpg_cross;
	bool branch_succeeds;
	bool page_crossed;
	int CYCLE(int n) { if (page_crossed) return (n + 1); else return n; }
	int CYCLE_BRANCH(int n) { if (branch_succeeds) return (n + 1); else return n; }
	int CYCLE_CROSS() { if (page_crossed) return (2); else return 0; }

	//reset vector
	void reset();
	void irq();
	void nmi();

	//addr modes
	uint16_t accumulator();
	uint16_t implied();
	uint16_t immediate();
	uint16_t relative();
	uint16_t zeropage();
	uint16_t zeropage_x();
	uint16_t zeropage_y();
	uint16_t absolute();
	uint16_t absolute_x();
	uint16_t absolute_y();
	uint16_t indirect();
	uint16_t indirect_x();
	uint16_t indirect_y();

	//opcodes
	void OP_LDA(uint16_t);
	void OP_LDY(uint16_t);
	void OP_LDX(uint16_t);
	void OP_STA(uint16_t);
	void OP_STX(uint16_t);
	void OP_STY(uint16_t);
	void OP_CLC(uint16_t);
	void OP_CLI(uint16_t);
	void OP_CLD(uint16_t);
	void OP_CLV(uint16_t);
	void OP_SEC(uint16_t);
	void OP_SEI(uint16_t);
	void OP_SED(uint16_t);
	void OP_JSR(uint16_t);
	void OP_RTS(uint16_t);
	void OP_JMP(uint16_t);
	void OP_TXA(uint16_t);
	void OP_TAX(uint16_t);
	void OP_TAY(uint16_t);
	void OP_TSX(uint16_t);
	void OP_TXS(uint16_t);
	void OP_TYA(uint16_t);
	void OP_NOP(uint16_t);
	void OP_DEC(uint16_t);
	void OP_DEX(uint16_t);
	void OP_DEY(uint16_t);
	void OP_INX(uint16_t);
	void OP_INY(uint16_t);
	void OP_INC(uint16_t);
	void OP_CMP(uint16_t);
	void OP_CPX(uint16_t);
	void OP_CPY(uint16_t);
	void OP_BCC(uint16_t);
	void OP_BCS(uint16_t);
	void OP_BEQ(uint16_t);
	void OP_BMI(uint16_t);
	void OP_BNE(uint16_t);
	void OP_BPL(uint16_t);
	void OP_BVC(uint16_t);
	void OP_BVS(uint16_t);
	void OP_ADC(uint16_t);
	void OP_SBC(uint16_t);
	void OP_AND(uint16_t);
	void OP_EOR(uint16_t);
	void OP_ORA(uint16_t);
	void OP_BRK(uint16_t);
	void OP_RTI(uint16_t);
	void OP_PHA(uint16_t);
	void OP_PHP(uint16_t);
	void OP_PLP(uint16_t);
	void OP_PLA(uint16_t);
	void OP_BIT(uint16_t);
	void OP_ROL(uint16_t, int);
	void OP_ROR(uint16_t, int);
	void OP_LSR(uint16_t, int);
	void OP_ASL(uint16_t, int);

	//fetch-decode-execute
	int step_instruction();
	void runfor(int);
	uint8_t decode(uint8_t);
};

#endif // CPU6502_H  