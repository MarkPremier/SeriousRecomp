"""Exercise the translator's emitted operations and the user's CRT sine path."""
from pathlib import Path
import sys

sys.path.insert(0, sys.argv[2])
from tools.recomp.disasm import Instruction, Operand
from tools.recomp.lifter import Lifter

source = Path(sys.argv[1]).read_text()
start = source.index("void sub_002B50F1(void)")
out = source[start:source.index("\n/**", start)]
out += "\n#define fp_top() g_fp_stack[g_fp_top]\n#define fp_st1() g_fp_stack[(g_fp_top+1)&7]\n"
out += "#define fp_push(v) do { double value=(v);g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=value;} while(0)\n"
out += "\nstatic void emitted_math(unsigned operation) {\n switch(operation) {\n"
for index, mnemonic in enumerate(("fsin", "fcos", "fsincos", "fptan", "fscale")):
    instruction = Instruction(0, 2, mnemonic, "", "")
    out += f"case {index}: " + " ".join(Lifter().lift_instruction(instruction)) + " break;\n"
out += "} }\n"
out += "\nstatic uint32_t emitted_shift(unsigned kind, unsigned bits, unsigned count, unsigned carry, unsigned *after) {\n"
out += "int _cf=(int)carry;uint32_t _fa=0,_fb=0;int32_t _fas=0,_fbs=0;\n switch(kind*3+(bits==8?0:bits==16?1:2)) {\n"
for kind, mnemonic in enumerate(("shl", "shr")):
    for index, (register, bits) in enumerate((("al", 8), ("ax", 16), ("eax", 32))):
        operands = [Operand(type="reg", reg=register), Operand(type="reg", reg="cl")]
        instruction = Instruction(0, 2, mnemonic, "", "", operands=operands)
        lifter = Lifter(); lifter.needs_cf = True
        out += f"case {kind*3+index}: " + " ".join(lifter.lift_instruction(instruction)) + " break;\n"
out += "} *after=(unsigned)_cf;return g_eax; }\n"
# Constant expressions must be safe too: compilers may fold undefined shifts
# differently from the hardware instruction used for a variable count.
out += "static uint32_t emitted_immediate_shift(void) {int _cf=1;uint32_t _fa=0,_fb=0;int32_t _fas=0,_fbs=0;\n"
instruction = Instruction(0, 3, "shr", "eax, 61", "", operands=[Operand(type="reg", reg="eax"), Operand(type="imm", imm=61)])
lifter = Lifter(); lifter.needs_cf = True
out += " ".join(lifter.lift_instruction(instruction)) + "return g_eax;}\n"
Path(sys.argv[3]).write_text(out)
