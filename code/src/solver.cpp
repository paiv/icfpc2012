
namespace paiv {


s32
solve(istream& si, ostream& so) {
    auto map = read_map(si);

    // program_t prog = read_program("DDDLLLLLLURRRRRRRRRRRRDDDDDDDLLLLLLLLLLLDDDRRRRRRRRRRRD");
    program_t prog = read_program("LLDLDRRD");

    so << prog;

    return 0;
}


}
