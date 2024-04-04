import java.io.*;

public class vmsim {
  public static void main(String[] args) throws NumberFormatException, IOException {
    if (args.length != 9) {
      // Inputted number of args was not 9
      System.out.println("Inputted " + args.length + " argument(s). 9 arguments needed.");
      System.exit(-1);
    }

    String algorithm = null;
    if ("opt*".equals(args[1]) || "lfu".equals(args[1])) {
      // Using either OPT* or LFU
      algorithm = args[1];
    } else {
      // Invalid algorithm passed in
      System.out.println("'" + args[1] + "' is invalid. Please choose 'opt*' or 'lfu'.");
      System.exit(2);
    }

    /* Args */
    int nFrames = 0; // Number of page frames
    int pageSize = 0; // Size of a page in KB
    String[] split = null; // Splits frame ratio for both processes
    int process1 = 0; // Ratio for process 1
    int process2 = 0; // Ratio for process 2
    int nFrames1 = 0; // Calculated number of frames for process 1
    int nFrames2 = 0; // Calculated number of frames for process 2
    String fName = null; // Name of trace file 

    /* Reading the trace file */
    PRAlgo result = null; // Stores statistics of OPT* or LFU
    File traceFile = null; // Object of trace file
    BufferedReader bf = null; // Reads the trace file memory accesses
    String line; // Reads each line of a trace file
    int index = 0; // line # of trace file

    /* Trace file information */
    char mode = 0; // 'l' = load, 's' = store (dirty)
    long address = 0; // page number given in trace file
    int process = 0; // 0 = process1, 1 = process2
    

    // Parse args
    try {
      // Get frames and size of each frame
      nFrames = Integer.parseInt(args[3]);
      pageSize = Integer.parseInt(args[5]);

      // Split frame ratio between both processes
      split = args[7].split(":");
      process1 = Integer.parseInt(split[0]);
      process2 = Integer.parseInt(split[1]);
      nFrames1 = (int) ((double) nFrames * (double) process1 / ((double) process1 + (double) process2));
      nFrames2 = nFrames - nFrames1;

      // Get trace file
      fName = args[8];
      traceFile = new File(fName);
      bf = new BufferedReader(new FileReader(traceFile));
    } catch (NumberFormatException | ArrayIndexOutOfBoundsException | FileNotFoundException e) {
      // Error occured getting trace file
      if (e instanceof NumberFormatException || e instanceof ArrayIndexOutOfBoundsException) {
        // Invalid args
        System.out.println("Usage: ./vmsim -a <opt*|lfu> -n <numframes> -p <pagesize in KB> -s <memory split> <tracefile>");
      } else if (e instanceof FileNotFoundException) {
        // No file
        System.out.println("File not found: " + fName);
      }
      System.exit(-1);
    }

    // Run algorithm
    if (algorithm.equals("opt*")) {
      // OPT*
      result = new OPTStar(nFrames, pageSize, process1, process2, traceFile, nFrames1, nFrames2);
    } else {
      // LFU
      result = new LFU(nFrames, pageSize, process1, process2, nFrames1, nFrames2);
    }

    while ((line = bf.readLine()) != null) {
      // Get mode, page number, and what process it goes to
      String[] data = line.split(" ");
      mode = data[0].charAt(0);
      address = Long.decode(data[1]) / (pageSize * 1024); 
      process = Integer.parseInt(data[2]);

      // Simulate memory request
      result.runAlgorithm(mode, address, process, index);
      index++;
    }
    System.out.println(result);
  }
}

interface PRAlgo {
  public void runAlgorithm(char accessType, long address, int process, int line);
  public String toString();
}