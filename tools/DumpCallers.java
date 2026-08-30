import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.*;
import java.io.*;
import java.util.*;

public class DumpCallers extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        String OUT = "/tmp/gdecomp";
        new File(OUT).mkdirs();
        DecompInterface di = new DecompInterface();
        di.setOptions(new DecompileOptions());
        di.toggleCCode(true);
        di.openProgram(currentProgram);
        for (String a : args) {
            Address ad = currentProgram.getAddressFactory().getAddress(a);
            Function f = getFunctionAt(ad);
            if (f == null) f = getFunctionContaining(ad);
            if (f == null) { disassemble(ad); createFunction(ad, "forced_" + a); f = getFunctionAt(ad); }
            if (f == null) { println("no func @ " + a); continue; }
            ReferenceManager rm = currentProgram.getReferenceManager();
            Set<Function> callers = new LinkedHashSet<>();
            for (Reference r : rm.getReferencesTo(f.getEntryPoint())) {
                Function cf = getFunctionContaining(r.getFromAddress());
                if (cf != null) callers.add(cf);
            }
            for (Function cf : callers) println("CALLER of " + a + " : " + cf.getEntryPoint());
            // decompila ogni caller
            for (Function cf : callers) {
                DecompileResults r = di.decompileFunction(cf, 120, monitor);
                String c = r.getDecompiledFunction() != null ? r.getDecompiledFunction().getC() : "// FAIL";
                try (PrintWriter w = new PrintWriter(new FileWriter(OUT + "/" + cf.getEntryPoint() + ".c"))) {
                    w.print(c);
                }
                println("decompiled caller " + cf.getEntryPoint() + " len=" + c.length());
            }
        }
        di.dispose();
        println("DONE callers");
    }
}
