import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import java.io.*;

public class DumpRange extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] a = getScriptArgs();
        long lo = Long.parseLong(a[0], 16);
        long hi = Long.parseLong(a[1], 16);
        String OUT = "/tmp/grange_" + a[0] + ".c";
        DecompInterface di = new DecompInterface();
        di.setOptions(new DecompileOptions());
        di.toggleCCode(true);
        di.openProgram(currentProgram);
        PrintWriter w = new PrintWriter(new FileWriter(OUT));
        int n = 0;
        Address p = toAddr(lo);
        Address end = toAddr(hi);
        while (p.compareTo(end) < 0) {
            Function f = getFunctionContaining(p);
            if (f == null) {
                // forza creazione a ogni etichetta/branch target noto? prova blind ogni 4
                p = p.add(4);
                continue;
            }
            DecompileResults r = di.decompileFunction(f, 90, monitor);
            if (r.getDecompiledFunction() != null) {
                w.println("//==== " + f.getEntryPoint() + " ====");
                w.println(r.getDecompiledFunction().getC());
                n++;
            }
            p = f.getEntryPoint().add(f.getBody().getNumAddresses());
        }
        w.close(); di.dispose();
        println("DONE " + n + " functions -> " + OUT);
    }
}
