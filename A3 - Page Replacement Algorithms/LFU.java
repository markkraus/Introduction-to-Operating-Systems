import java.util.HashMap;
import java.util.HashSet;

class LFU implements PRAlgo {
  Process[] processes = new Process[2];
  int pageSize; // Size of page in KB
  int nFrames; // Number of frames
  int memAccesses = 0; // Total memory requests
  int faults = 0; // Total page faults
  int writes = 0; // Total writes to disk

  public LFU(int nFrames, int pageSize, int process1, int process2, int nFrames1, int nFrames2) {
    this.nFrames = nFrames;
    this.pageSize = pageSize;
    processes[0] = new Process(nFrames1);
    processes[1] = new Process(nFrames2);
  }

  /**
   * Runs the LFU page replacement algorithm for a memory request.
   *
   * @param mode    's' (store) or 'l' (load) mode
   * @param address Address where memory request was made
   * @param process 0 for process 1, 1 for process 2
   * @param index   The index of the memory request
   */
  public void runAlgorithm(char mode, long address, int process, int index) {
    Process currentProcess = processes[process];
    PTE page = currentProcess.pageTable.get(address);

    if (page == null) {
      // Page is not in the cache - add it
      handlePageFault(currentProcess, mode, address, index);
    } else {
      // Page is in the cache - update attributes and access history
      if (mode == 's') {
        page.dirty = true;
      }
      page.nAccesses++;
      page.lastAccess = index;
    }
    memAccesses++;
  }

  /**
   * Handles a page fault by adding the requested page to the cache.
   *
   * @param currentProcess The current process
   * @param mode           's' (store) or 'l' (load) mode
   * @param address        Address where memory request was made
   * @param index          The index of the memory request
   */
  private void handlePageFault(Process currentProcess, char mode, long address, int index) {
    faults++;
    PTE page = new PTE(address);
    if (mode == 's') {
      page.dirty = true;
    }
    if (currentProcess.cache.size() < currentProcess.size) {
      // Cache has free space - add page
      currentProcess.cache.add(address);
    } else {
      /// Cache is full - select victim to evict
      selectVictim(currentProcess);
      currentProcess.cache.add(address);
    }
    currentProcess.pageTable.put(address, page);
    page.lastAccess = index; // Updates access history
  }

  /**
   * Selects a victim page from the cache for replacement.
   *
   * @param process The process from which to select the victim page
   */
  private void selectVictim(Process process) {
    PTE leastUsedPage = getLeastUsedPage(process);
    if (leastUsedPage.dirty) {
      writes++;
    }
    process.cache.remove(leastUsedPage.address);
    process.pageTable.remove(leastUsedPage.address);
  }

  /**
   * Finds and returns the least frequently used page in the cache.
   *
   * @param process The process to search for the least used page
   * @return The least used page in the cache
   */
  private PTE getLeastUsedPage(Process process) {
    PTE leastUsedPage = null;
    int minAccessCount = Integer.MAX_VALUE;

    for (Long address : process.cache) {
      PTE page = process.pageTable.get(address);
      if (page.nAccesses < minAccessCount
          || (page.nAccesses == minAccessCount && page.lastAccess < leastUsedPage.lastAccess)) {
        leastUsedPage = page;
        minAccessCount = page.nAccesses;
      }
    }
    return leastUsedPage;
  }

  public String toString() {
    return "Algorithm: LFU\n" +
        "Number of frames: " + nFrames + "\n" +
        "Page size: " + pageSize + " KB\n" +
        "Total memory accesses: " + memAccesses + "\n" +
        "Total page faults: " + faults + "\n" +
        "Total writes to disk: " + writes;
  }

  private class Process {
    int size; // Number of frames allocated to the process
    HashSet<Long> cache; // Cache storing page addresses
    HashMap<Long, PTE> pageTable; // Page table mapping page addresses to page table entries

    private Process(int nFrames) {
      size = nFrames;
      cache = new HashSet<>();
      pageTable = new HashMap<>();
    }
  }

  private class PTE {
    long address; // Address of the page
    int nAccesses; // Number of times the page has been accessed
    int lastAccess; // Index of the last access
    boolean dirty; // Indicates whether the page has been modified

    private PTE(long address) {
      this.address = address;
      this.nAccesses = 1;
      this.lastAccess = 0;
      this.dirty = false;
    }
  }
}
