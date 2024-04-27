#define FUSE_USE_VERSION 26

#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <stdio.h>
#include <string.h>

#include "cs1550.h"

/**
 * Global Variables:
 */
struct cs1550_root_directory *root; // Root block
FILE *file;                         //.disk file

/**
 * Helper Functions:
 */

/**
 * This function validates whether a given path conforms to certain criteria.
 * It checks if the length of the directory name, file name, and extension
 * is within the maximum allowed lengths.
 * @param path The path to validate
 * @return 1 if the path is valid, 0 otherwise
 */
static int validate_path(const char *path)
{
  int valid_directory = 0; // 1 - valid directory, 0 - invalid
  int delimiter_index = 0; // Index of "/" or "." in the path
  int valid_file = 0;      // 1 - valid file, 0 - invalid

  // Count characters for the directory name
  for (int i = 1; i < (MAX_FILENAME + 2); i++)
  {
    if (path[i] == '/')
    {
      // Delimter character - valid directory name
      valid_directory = 1;
      delimiter_index = i; // Store delimiter location
      break;
    }
    else if (path[i] == '\0')
    {
      // End directory name - valid
      return 1;
    }
  }

  if (valid_directory == 0)
  {
    // No file name or exceeds MAX - invalid
    return 0;
  }

  // Count characters for file name
  for (int i = delimiter_index + 1; i < (delimiter_index + MAX_FILENAME + 2); i++)
  {
    if (path[i] == '.')
    {
      // Delimiter character - valid file name
      valid_file = 1;
      delimiter_index = i; // Store delimiter location
      break;
    }
    else if (path[i] == '\0')
    {
      // End file name - valid
      return 1;
    }
  }

  if (valid_file == 0)
  {
    // No file or exceeds MAX - invalid
    return 0;
  }

  // Count characters for extension
  for (int i = delimiter_index + 1; i < (delimiter_index + MAX_EXTENSION + 2); i++)
  {
    if (path[i] == '\0')
    {
      // End extension name - valid
      return 1;
    }
  }

  // Invalid directory, file, or extension name
  return 0;
}

/**
 * This function searches for a directory entry with the specified name in the root directory.
 * If a match is found, it reads the corresponding directory block from the .disk file.
 * @param target_directory The name of the directory to search for
 * @return A pointer to the directory entry if found, otherwise NULL
 */
static struct cs1550_directory_entry *find_directory_match(char target_directory[])
{
  // Iterate over the array of directory entries using the number of directories as the number of iterations
  for (size_t i = 0; i < root->num_directories; i++)
  {
    // Compare the directory name from the path with the directory name in the entry
    if (strcmp(target_directory, root->directories[i].dname) == 0)
    {
      // Directory name matches - get directory's starting block & allocate memory for the pointer to it
      int start = root->directories[i].n_start_block;
      struct cs1550_directory_entry *matched_directory = malloc(sizeof(struct cs1550_directory_entry));
      if (!matched_directory)
      {
        // Could not allocate memory
        return NULL;
      }

      // Go to starting block and read the data into entry
      fseek(file, start * BLOCK_SIZE, SEEK_SET);
      fread(matched_directory, BLOCK_SIZE, 1, file);
      return matched_directory;
    }
  }

  // Unable to find a matching directory
  return NULL;
}

/**
 *
 */
static struct cs1550_file_entry *find_file_match(struct cs1550_directory_entry *directory, char target_file[], char extension[])
{
  for (size_t i = 0; i < directory->num_files; i++)
  {
    // Check if the file names match
    if ((strcmp(target_file, directory->files[i].fname) == 0) && (strcmp(extension, directory->files[i].fext) == 0))
    {
      return &(directory->files[i]);
    }
  }

  // Unable to find a matching file
  return NULL;
}

static int get_head_block(char directory[])
{
  // Loop through root directories
  for (size_t i = 0; i < root->num_directories; i++)
  {
    // Check if any of the directories match the requested one
    if (strcmp(directory, root->directories[i].dname) == 0)
    {
      // Copy the starting block number into an int
      return root->directories[i].n_start_block;
    }
  }

  // Unable to find a matching directory
  return 0;
}

/**
 * Called whenever the system wants to know the file or directory attributes, including
 * simply whether the file/directory exists or not.
 *
 * `man 2 stat` will show the fields of a `struct stat` structure.
 */
static int cs1550_getattr(const char *path, struct stat *statbuf)
{
  // Clear out `statbuf` first -- this function initializes it.
  memset(statbuf, 0, sizeof(struct stat));

  // Validate path
  if (validate_path(path) == 0)
  {
    // Directory, file, or extension name was invalid
    return -ENAMETOOLONG;
  }

  // Check if the path is the root directory.
  if (strcmp(path, "/") == 0)
  {
    statbuf->st_mode = S_IFDIR | 0755;
    statbuf->st_nlink = 2;

    return 0; // no error
  }

  // Parse the path
  char directory[MAX_FILENAME + 1];
  char filename[MAX_FILENAME + 1];
  char extension[MAX_EXTENSION + 1];
  int result = sscanf(path, "/%[^/]/%[^.].%s", directory, filename, extension);

  // Check if the path is a subdirectory.
  if (result == 1)
  {
    // Attempt to read the root directory block
    struct cs1550_directory_entry *matching_directory = find_directory_match(directory);
    if (matching_directory)
    {
      // Directory found - Return that it is a directory with given permissions and with 2 links
      statbuf->st_mode = S_IFDIR | 0755;
      statbuf->st_nlink = 2;

      free(matching_directory);
      return 0; // no error
    }
  }

  // Check if the path is a file.
  if (result == 2 || result == 3)
  {
    // Attempt to find matching directory in the root block
    struct cs1550_directory_entry *matching_directory = find_directory_match(directory);
    if (!matching_directory)
    {
      return -ENOENT;
    }
    else
    {
      // If we find a match, attempt to find a matching file
      struct cs1550_file_entry *matching_file = find_file_match(matching_directory, filename, extension);
      if (!matching_file)
      {
        free(matching_directory);
        return -ENOENT;
      }
      // Regular file
      statbuf->st_mode = S_IFREG | 0666;

      // Only one hard link to this file
      statbuf->st_nlink = 1;

      statbuf->st_size = matching_file->fsize;
    }

    free(matching_directory);
    return 0; // no error
  }

  // Otherwise, the path doesn't exist.
  return -ENOENT;
}

/**
 * Creates a directory. Ignore `mode` since we're not dealing with permissions.
 * Check the pseudo-code in recitation slides.
 */
static int cs1550_mkdir(const char *path, mode_t mode)
{
  (void)mode; // unused

  // Validate path
  if (validate_path(path) == 0)
  {
    // Directory, file, or extension name was invalid
    return -ENAMETOOLONG;
  }

  // Parse the path
  char directory[MAX_FILENAME + 1];
  char filename[MAX_FILENAME + 1];
  char extension[MAX_EXTENSION + 1];
  int result = sscanf(path, "/%[^/]/%[^.].%s", directory, filename, extension);

  if (result == 1)
  {
    // Validate the directory does not exist
    for (size_t i = 0; i < root->num_directories; i++)
    {
      if (strcmp(directory, root->directories[i].dname) == 0)
      {
        // Directory already exists
        return -EEXIST;
      }
    }

    // Check if maximum number of directories already hit
    if (root->num_directories < MAX_DIRS_IN_ROOT)
    {
      // Copy the directory name from the path to the directory name at entry at index num_directories
      strncpy(root->directories[root->num_directories].dname, directory, (MAX_FILENAME + 1));

      // Use the last_allocated_block+1 as the block number for the new directory and increment last_allocated_block
      root->directories[root->num_directories].n_start_block = root->last_allocated_block + 1;
      root->last_allocated_block++;

      // Increment num_directories
      root->num_directories++;

      // Seek to the beginning of the file and write the root directory back to the file
      fseek(file, 0, SEEK_SET);
      fwrite(root, BLOCK_SIZE, 1, file);

      return 0; // Return success
    }
    else
    {
      // MAX hit
      return -ENOSPC;
    }
  }

  // Directory is not under root directory
  return -EPERM;
}

/**
 * Does the actual creation of a file. `mode` and `dev` can be ignored.
 * Check the pseudo-code in recitation slides.
 */
static int cs1550_mknod(const char *path, mode_t mode, dev_t dev)
{
  (void)mode; // unused
  (void)dev;  // unused

  // Check if the path is valid
  if (validate_path(path) == 0)
  {
    return -ENAMETOOLONG;
  }

  char directory[MAX_FILENAME + 1];
  char filename[MAX_FILENAME + 1];
  char extension[MAX_EXTENSION + 1];
  int result = sscanf(path, "/%[^/]/%[^.].%s", directory, filename, extension);

  if (result == 2 || result == 3)
  {
    // Attempt to read the root directory block
    struct cs1550_directory_entry *matching_directory = find_directory_match(directory);
    if (matching_directory)
    {
      // Directory found - Attempt to find a matching file in the directory
      struct cs1550_file_entry *matching_file = find_file_match(matching_directory, filename, extension);

      if (!matching_file)
      {
        // File doesn't exist yet - check if maximum files in directory already hit
        if (matching_directory->num_files < MAX_FILES_IN_DIR)
        {
          // Get the head of directory
          int head = get_head_block(directory);

          // Copy the file name (and extension if exists) from the path to the file name at entry at index num_files
          strncpy(matching_directory->files[matching_directory->num_files].fname, filename, (MAX_FILENAME + 1));
          if (result == 3)
          {
            // Extension exists
            strncpy(matching_directory->files[matching_directory->num_files].fext, extension, (MAX_EXTENSION + 1));
          }
          matching_directory->files[matching_directory->num_files].fsize = 0;

          // Use the last_allocated_block+1 from the root block as the block number for the index block and increment last_allocated_block
          matching_directory->files[matching_directory->num_files].n_index_block = root->last_allocated_block + 1;
          root->last_allocated_block++;

          // Seek to and read the index block into a variable of type struct cs1550_index_block
          struct cs1550_index_block *index = malloc(sizeof(struct cs1550_index_block));
          memset(index, 0, sizeof(struct cs1550_index_block));
          fseek(file, ((root->last_allocated_block + 1) * BLOCK_SIZE), SEEK_SET);
          fread(index, BLOCK_SIZE, 1, file);

          // Write the value of the last_allocated_block as the first entry in the index block array
          index->entries[0] = root->last_allocated_block + 1;

          // Increment last_allocated_block for the first data block of the file
          root->last_allocated_block++;

          // Increment num_files
          matching_directory->num_files++;

          // Seek to the beginning of the file and write the root directory back to the file
          fseek(file, 0, SEEK_SET);
          fwrite(root, BLOCK_SIZE, 1, file);

          // Seek to the file directory block location and write the file directory back to the file
          fseek(file, head * BLOCK_SIZE, SEEK_SET);
          fwrite(matching_directory, BLOCK_SIZE, 1, file);

          // Seek to the index block location and write the index block back to the file
          fseek(file, ((root->last_allocated_block - 1) * BLOCK_SIZE), SEEK_SET);
          fwrite(index, BLOCK_SIZE, 1, file);

          free(matching_directory);
          free(matching_file);
          free(index);
          return 0;
        }
        else
        {
          // Max files in directory hit
          free(matching_directory);
          return -ENOSPC;
        }
      }
      else
      {
        // File already exists
        free(matching_directory);
        return -EEXIST;
      }
    }
    else
    {
      // Unable to find matching directory
      return -ENOENT;
    }
  }
  else
  {
    // No file or extension scanned in
    return -EPERM;
  }
}

/**
 * Called whenever the contents of a directory are desired. Could be from `ls`,
 * or could even be when a user presses TAB to perform autocompletion.
 */
static int cs1550_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                          off_t offset, struct fuse_file_info *fi)
{
  (void)offset; // unused
  (void)fi;     // unused

  // Validate the path
  if (validate_path(path) == 0)
  {
    return -ENAMETOOLONG;
  }

  // Parse the path
  char directory[MAX_FILENAME + 1];
  char filename[MAX_FILENAME + 1];
  char extension[MAX_EXTENSION + 1];
  int result = sscanf(path, "/%[^/]/%[^.].%s", directory, filename, extension);

  if (strcmp(path, "/") == 0)
  {
    // Directory path
    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    // Add directories in root to directory listing
    for (size_t i = 0; i < root->num_directories; i++)
    {
      filler(buf, root->directories[i].dname, NULL, 0);
    }

    return 0; // no error
  }
  else if (result == 1)
  {
    // Subdirectory
    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    // Attempt to find matching directory in the root block
    struct cs1550_directory_entry *matching_directory = find_directory_match(directory);
    if (matching_directory)
    {
      // Found directory - Iterate over the entries in the file directory and use the filler function to insert the file name and
      // extension(if it exists) into the provided buffer
      char file[MAX_FILENAME + MAX_EXTENSION + 2];
      for (size_t i = 0; i < matching_directory->num_files; i++)
      {
        strncpy(file, matching_directory->files[i].fname, (MAX_FILENAME + 1));
        if (strcmp(matching_directory->files[i].fext, "") != 0)
        {
          strncat(file, ".", 2);
          strncat(file, matching_directory->files[i].fext, (MAX_EXTENSION + 1));
        }

        filler(buf, file, NULL, 0);
      }

      free(matching_directory);
      return 0;
    }
    else
    {
      // Unable to find matching directory
      return -ENOENT;
    }
  }
  else
  {
    // Permission error
    return -ENOTDIR;
  }
}

/**
 * Removes a directory.
 */
static int cs1550_rmdir(const char *path)
{
  // Validate path
  if (validate_path(path) == 0)
  {
    // Directory, file, or extension name was invalid
    return -ENAMETOOLONG;
  }

  // Parse the path
  char directory[MAX_FILENAME + 1];
  char filename[MAX_FILENAME + 1];
  char extension[MAX_EXTENSION + 1];
  int result = sscanf(path, "/%[^/]/%[^.].%s", directory, filename, extension);

  // Check if the path is a subdirectory.
  if (result == 1)
  {
    // Read the root directory block into a variable
    fseek(file, 0, SEEK_SET);
    fread(root, BLOCK_SIZE, 1, file);

    // Iterate over the array of directory entries
    for (size_t i = 0; i < root->num_directories; i++)
    {
      // Compare the directory name from the path with the directory name in the entry
      if (strncmp(directory, root->directories[i].dname, MAX_FILENAME + 1) == 0)
      {
        // Directory found
        int block_number = root->directories[i].n_start_block;

        // Read the file directory block into a variable
        struct cs1550_directory_entry *dir_entry = malloc(sizeof(struct cs1550_directory_entry));
        fseek(file, block_number * BLOCK_SIZE, SEEK_SET);
        fread(dir_entry, BLOCK_SIZE, 1, file);

        // Check if the directory is empty
        if (dir_entry->num_files > 0)
        {
          free(dir_entry);
          return -ENOTEMPTY; // Directory not empty
        }

        // Remove the directory from the root directory
        for (size_t j = i; j < root->num_directories - 1; j++)
        {
          root->directories[j] = root->directories[j + 1];
        }
        root->num_directories--;

        // Write the root directory back to the .disk file
        fseek(file, 0, SEEK_SET);
        fwrite(root, BLOCK_SIZE, 1, file);

        free(dir_entry);
        return 0; // Success
      }
    }

    // Directory not found
    return -ENOENT;
  }

  // Not a subdirectory
  return -ENOTDIR;
}

/**
 * Deletes a file.
 */
static int cs1550_unlink(const char *path)
{
  // Validate path
  if (validate_path(path) == 0)
  {
    // Directory, file, or extension name was invalid
    return -ENAMETOOLONG;
  }

  // Parse the path
  char directory[MAX_FILENAME + 1];
  char filename[MAX_FILENAME + 1];
  char extension[MAX_EXTENSION + 1];
  int result = sscanf(path, "/%[^/]/%[^.].%s", directory, filename, extension);

  // Check if the path is a file.
  if (result == 2 || result == 3)
  {
    // Read the root directory block into a variable
    fseek(file, 0, SEEK_SET);
    fread(root, BLOCK_SIZE, 1, file);

    // Iterate over the array of directory entries
    for (size_t i = 0; i < root->num_directories; i++)
    {
      // Compare the directory name from the path with the directory name in the entry
      if (strncmp(directory, root->directories[i].dname, MAX_FILENAME + 1) == 0)
      {
        // Directory found
        int block_number = root->directories[i].n_start_block;

        // Read the file directory block into a variable
        struct cs1550_directory_entry *dir_entry = malloc(sizeof(struct cs1550_directory_entry));
        fseek(file, block_number * BLOCK_SIZE, SEEK_SET);
        fread(dir_entry, BLOCK_SIZE, 1, file);

        // Iterate over the array of file entries
        for (size_t j = 0; j < dir_entry->num_files; j++)
        {
          // Compare the file name (and extension if exists) from the path with the file name (and extension if exists) in the entry
          if (strncmp(filename, dir_entry->files[j].fname, MAX_FILENAME + 1) == 0 &&
              (result == 2 || strncmp(extension, dir_entry->files[j].fext, MAX_EXTENSION + 1) == 0))
          {
            // File found

            // Remove the file from the directory entry
            for (size_t k = j; k < dir_entry->num_files - 1; k++)
            {
              dir_entry->files[k] = dir_entry->files[k + 1];
            }
            dir_entry->num_files--;

            // Write the file directory back to the file
            fseek(file, block_number * BLOCK_SIZE, SEEK_SET);
            fwrite(dir_entry, BLOCK_SIZE, 1, file);

            free(dir_entry);
            return 0; // Success
          }
        }

        free(dir_entry);
        return -ENOENT; // File not found
      }
    }

    // Directory not found
    return -ENOENT;
  }

  // Not a file
  return -ENOTDIR;
}

/**
 * Called when a new file is created (with a 0 size) or when an existing file
 * is made shorter or longer.
 */
static int cs1550_truncate(const char *path, off_t size)
{
  // Validate path
  if (validate_path(path) == 0)
  {
    // Directory, file, or extension name was invalid
    return -ENAMETOOLONG;
  }

  // Parse the path
  char directory[MAX_FILENAME + 1];
  char filename[MAX_FILENAME + 1];
  char extension[MAX_EXTENSION + 1];
  int result = sscanf(path, "/%[^/]/%[^.].%s", directory, filename, extension);

  // Check if the path is a file
  if (result == 2 || result == 3)
  {
    // Read the root directory block into a variable
    fseek(file, 0, SEEK_SET);
    fread(root, BLOCK_SIZE, 1, file);

    // Iterate over the array of directory entries
    for (size_t i = 0; i < root->num_directories; i++)
    {
      // Compare the directory name from the path with the directory name in the entry
      if (strncmp(directory, root->directories[i].dname, MAX_FILENAME + 1) == 0)
      {
        // Directory found
        int block_number = root->directories[i].n_start_block;

        // Read the file directory block into a variable
        struct cs1550_directory_entry *dir_entry = malloc(sizeof(struct cs1550_directory_entry));
        fseek(file, block_number * BLOCK_SIZE, SEEK_SET);
        fread(dir_entry, BLOCK_SIZE, 1, file);

        // Iterate over the array of file entries
        for (size_t j = 0; j < dir_entry->num_files; j++)
        {
          // Compare the file name (and extension) from the path with the file name (and extension) in the entry
          if (strncmp(filename, dir_entry->files[j].fname, MAX_FILENAME + 1) == 0 &&
              strncmp(extension, dir_entry->files[j].fext, MAX_EXTENSION + 1) == 0)
          {
            // File found
            if (size < 0)
            {
              free(dir_entry);
              return -EINVAL; // Negative size is invalid
            }
            else
            {
              // Update the file size
              dir_entry->files[j].fsize = size;

              // Write the file directory back to the file
              fseek(file, block_number * BLOCK_SIZE, SEEK_SET);
              fwrite(dir_entry, BLOCK_SIZE, 1, file);

              free(dir_entry);
              return 0; // Success
            }
          }
        }

        free(dir_entry);
        return -ENOENT; // File not found
      }
    }

    // Directory not found
    return -ENOENT;
  }

  // Not a file
  return -ENOTDIR;
}

/**
 * This function should be used to open and/or initialize your `.disk` file.
 */
static void *cs1550_init(struct fuse_conn_info *fi)
{
  (void)fi;

  root = malloc(BLOCK_SIZE);
  file = fopen(".disk", "rb+");
  if (file != NULL)
  {
    fread(root, BLOCK_SIZE, 1, file);
  }
  return NULL;
}

/**
 * This function should be used to close the `.disk` file.
 */
static void cs1550_destroy(void *args)
{
  (void)args;
  // Free global variables
  free(root);
  fclose(file);
}

/*
 * Register our new functions as the implementations of the syscalls.
 */
static struct fuse_operations cs1550_oper = {
    .getattr = cs1550_getattr,
    .readdir = cs1550_readdir,
    .mkdir = cs1550_mkdir,
    .rmdir = cs1550_rmdir,
    .mknod = cs1550_mknod,
    .unlink = cs1550_unlink,
    .truncate = cs1550_truncate,
    .init = cs1550_init,
    .destroy = cs1550_destroy,
};

/*
 * Don't change this.
 */
int main(int argc, char *argv[])
{
  return fuse_main(argc, argv, &cs1550_oper, NULL);
}