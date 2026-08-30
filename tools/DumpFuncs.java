import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;
import java.io.*;
import java.util.*;
import java.util.regex.*;

public class DumpFuncs extends GhidraScript {
    String OUT = "/tmp/gdecomp";
    Set<String> done = new HashSet<>();

    Function funAt(String a) throws Exception {
        Address ad = currentProgram.getAddressFactory().getAddress(a);
        Function f = getFunctionAt(ad);
        if (f == null) f = getFunctionContaining(ad);
        if (f == null) {
            disassemble(ad);
            createFunction(ad, "forced_" + a);
            f = getFunctionAt(ad);
        }
        return f;
    }

    String decompile(Function f) throws Exception {
        DecompInterface di = new DecompInterface();
        DecompileOptions opts = new DecompileOptions();
        di.setOptions(opts);
        di.toggleCCode(true);
        di.openProgram(currentProgram);
        DecompileResults r = di.decompileFunction(f, 120, monitor);
        di.dispose();
        return r.getDecompiledFunction() != null ? r.getDecompiledFunction().getC() : "// FAIL\n";
    }

    List<String> callees(String c) {
        List<String> out = new ArrayList<>();
        Matcher m = Pattern.compile("FUN_([0-9a-f]{8})").matcher(c);
        while (m.find()) out.add(m.group(1));
        return out;
    }

    void dump(String addr, int depth) throws Exception {
        if (depth < 0 || done.contains(addr)) return;
        done.add(addr);
        Function f = funAt(addr);
        if (f == null) { println("no func @ " + addr); return; }
        String c = decompile(f);
        new File(OUT).mkdirs();
        try (PrintWriter w = new PrintWriter(new FileWriter(OUT + "/" + addr + ".c"))) {
            w.print(c);
        }
        println("decompiled " + addr + " depth=" + depth + " len=" + c.length());
        for (String a : callees(c)) dump(a, depth - 1);
    }

    @Override
    public void run() throws Exception {
        if (currentProgram.getImageBase().getOffset() != 0x100000L) {
            currentProgram.setImageBase(toAddr(0x100000L), true);
        }
        String[] args = getScriptArgs();
        int depth = args.length > 1 ? Integer.parseInt(args[1]) : 2;
        for (String a : args[0].split(",")) dump(a.trim(), depth);
        println("DONE " + done.size() + " functions -> " + OUT);
    }
}
