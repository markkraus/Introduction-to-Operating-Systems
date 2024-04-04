import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;

class OPTStar implements PRAlgo {
  Process[] processes = new Process[2]; // Stores memory requests for both processes
  HashMap<Long, PTE>[] memory; // Memory requests for both processes
  HashMap<Long, PTE> currentProcess; // Transient memory requests of given process
  int pageSize; // Page size in KB
  int nFrames; // Number of frames
  int nAccesses = 0; // Number of memory requests
  int faults = 0; // Total page faults
  int writes = 0; // Total writes to disk
  String line; // Contents of trace file line
  int offset; // Offset to remove for page number
  int index = 0; // # of lines read of trace file

  @SuppressWarnings("unchecked")
  public OPTStar(int nFrames, int pageSize, int process1, int process2, File file, int nFrames1, int nFrames2) throws FileNotFoundException {
    this.nFrames = nFrames;
    this.pageSize = pageSize;

    // Read each line of the trace file
    try (BufferedReader bf = new BufferedReader(new FileReader(file))) {
      // Maps each page number to the given process
      memory = new HashMap[2];
      memory[0] = new HashMap<>();
      memory[1] = new HashMap<>();

      while ((line = bf.readLine()) != null) {
        // Get address and what process it goes with
        String[] parts = line.split(" ");
        long address = Long.decode(parts[1]) / (pageSize * 1024);
        currentProcess = memory[Integer.parseInt(parts[2])];

        // Get the page number associated with current address
        PTE page = currentProcess.get(address);
        if (page == null) {
          // No page number for address - create a new page & store with associated process
          page = new PTE(address, index);
          currentProcess.put(address, page);
        }

        // Page was requested so update last accessed time-stamp
        page.accessHistory.add(index);
        index++;
      }

      processes[0] = new Process(nFrames1, memory[0]);
      processes[1] = new Process(nFrames2, memory[1]);
    } catch (IOException e) {
      // Some error occured reading the file
      System.exit(-2);
    }
  }

  /**
   * Runs the OPT* Page Replacement Algorithm.
   * 
   * @mode    's' (store) or 'l' (load) mode
   * @address Address where memory request was made
   * @process 0 for the process 1, 1 for process 2
   * @index   The line number of the trace file (zero-indexed)
   */
  public void runAlgorithm(char mode, long address, int process, int index) {
    Process currentProcess = processes[process];
    PTE page = currentProcess.pageTable.get(address);

    // Update the accessed time-stamp of page
    updatePageAccess(page, index, currentProcess);

    if (!currentProcess.cache.contains(address)) {
      // Address isn't in cache - page fault
      handlePageFault(currentProcess, page);
    }

    if (mode == 's') {
      // Page has been modified
      page.dirty = true;
    }
    nAccesses++;
  }

  /**
   * Updates the last referenced index of the given page.
   * 
   * @param page    Referenced page
   * @param index   The line number of the trace file (zero-indexed)
   * @param process 0 for the process 1, 1 for process 2
   */
  private void updatePageAccess(PTE page, int index, Process process) {
    page.accessHistory.remove(0);
    page.lastAccess = index;
  }

  /**
   * Updates variable values when a page fault occurs.
   * 
   * @param process 0 for the process 1, 1 for process 2
   * @param page    Referenced page
   */
  private void handlePageFault(Process process, PTE page) {
    faults++;
    if (process.cache.size() == process.size) {
      // Cache is full - evict a page
      Long victimAddress = selectVictim(process);
      process.cache.remove(victimAddress);
      if (process.pageTable.get(victimAddress).dirty) {
        // Victim has been modified - must write to disk
        writes++;
        process.pageTable.get(victimAddress).dirty = false;
      }
    }
    process.cache.add(page.address);
  }

  /**
   * Selects an address from the cache to be evicted.
   * 
   * @param process 0 for the process 1, 1 for process 2
   * @return an address from the process cache to be evicted
   */
  public Long selectVictim(Process process) {
    int minFutureAccessCount = Integer.MAX_VALUE;
    int leastRecentAccess = Integer.MAX_VALUE;

    // Loop through each address in the cache
    Long victimAddress = null;
    for (Long address : process.cache) {
      PTE page = process.pageTable.get(address);
      int futureAccesses = page.accessHistory.size();
      int lastAccess = page.lastAccess;
      boolean isBetterPage = futureAccesses < minFutureAccessCount || (futureAccesses == minFutureAccessCount && lastAccess < leastRecentAccess);

      if (isBetterPage) {
        // Page found that is accessed more recently than current "best" page
        victimAddress = address;
        minFutureAccessCount = futureAccesses;
        leastRecentAccess = lastAccess;
      }
    }
    return victimAddress;
  }

  public String toString() {
    return "Algorithm: OPT*\n" +
      "Number of frames: " + nFrames + "\n" +
      "Page size: " + pageSize + " KB\n" +
      "Total memory accesses: " + nAccesses + "\n" +
      "Total page faults: " + faults + "\n" +
      "Total writes to disk: " + writes;
  }

  private class PTE {
  ArrayList<Integer> accessHistory;
  int lastAccess; // Index of last memory request
  long address;
  boolean dirty; // page was written to or not
  
  public PTE(long address, int index) {
    lastAccess = index;
    this.address = address;
    dirty = false;
    accessHistory = new ArrayList<Integer>();
  }
}

  private class Process {
    HashSet<Long> cache;
    HashMap<Long, PTE> pageTable;
    int size;

    private Process(int nFrames, HashMap<Long, PTE> requests) {
      cache = new HashSet<>();
      pageTable = requests;
      size = nFrames;
    }
  }
}