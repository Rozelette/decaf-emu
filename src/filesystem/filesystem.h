#pragma once
#include "filesystem_file.h"
#include "filesystem_filehandle.h"
#include "filesystem_host_folder.h"
#include "filesystem_host_path.h"
#include "filesystem_link_file.h"
#include "filesystem_link_folder.h"
#include "filesystem_path.h"
#include "filesystem_virtual_folder.h"

namespace fs
{

class FileSystem
{
public:
   FileSystem() :
      mRoot("/")
   {
   }

   Folder *makeFolder(Path path)
   {
      auto node = createPath(path);

      if (!node || node->type != Node::FolderNode) {
         return nullptr;
      }

      return reinterpret_cast<Folder *>(node);
   }

   File *makeFile(Path path)
   {
      auto parent = createPath(path.parentPath());

      if (!parent || parent->type != Node::FolderNode) {
         return nullptr;
      }

      auto folder = reinterpret_cast<Folder *>(parent);
      auto node = folder->addFile(path.filename());

      if (!node || node->type != Node::FileNode) {
         return nullptr;
      }

      return reinterpret_cast<File *>(node);
   }

   bool deleteFolder(Path path)
   {
      auto parent = findNode(path.parentPath());

      if (!parent || parent->type != Node::FolderNode) {
         return false;
      }

      auto folder = reinterpret_cast<Folder *>(parent);
      return folder->deleteFolder(path.filename());
   }

   Node *makeLink(Path dst, Path src)
   {
      return makeLink(dst, findNode(src));
   }

   Node *makeLink(Path dst, Node *srcNode)
   {
      // Ensure src exists
      if (!srcNode) {
         return nullptr;
      }

      // Check to see if dst already exists
      auto dstNode = findNode(dst);

      if (dstNode) {
         return dstNode;
      }

      // Create parent path
      auto parent = createPath(dst.parentPath());

      if (!parent || parent->type != Node::FolderNode) {
         return nullptr;
      }

      auto folder = reinterpret_cast<Folder *>(parent);

      // Create link
      if (srcNode->type == Node::FolderNode) {
         dstNode = new FolderLink(reinterpret_cast<Folder *>(srcNode), dst.filename());
      } else if (srcNode->type == Node::FileNode) {
         dstNode = new FileLink(reinterpret_cast<File *>(srcNode), dst.filename());
      }

      if (dstNode) {
         dstNode = folder->addChild(dstNode);
      }

      return dstNode;
   }

   bool mountHostFolder(Path dst, HostPath src)
   {
      auto parent = createPath(dst.parentPath());

      if (!parent || parent->type != Node::FolderNode) {
         return false;
      }

      auto folder = reinterpret_cast<Folder *>(parent);
      return !!folder->addChild(new HostFolder(src, dst.filename()));
   }

   bool mountHostFile(Path dst, HostPath src)
   {
      auto parent = createPath(dst.parentPath());

      if (!parent || parent->type != Node::FolderNode) {
         return false;
      }

      auto folder = reinterpret_cast<Folder *>(parent);
      return !!folder->addChild(new HostFile(src, dst.filename()));
   }

   FileHandle *openFile(Path path, File::OpenMode mode)
   {
      auto node = findNode(path);

      if (!node || node->type != Node::FileNode) {
         return nullptr;
      }

      auto file = reinterpret_cast<File *>(node);
      return file->open(mode);
   }

   FolderHandle *openFolder(Path path)
   {
      auto node = findNode(path);

      if (!node || node->type != Node::FolderNode) {
         return nullptr;
      }

      auto folder = reinterpret_cast<Folder *>(node);
      return folder->open();
   }

   bool findEntry(Path path, FolderEntry &entry)
   {
      auto node = findNode(path);

      if (!node) {
         return false;
      }

      entry.name = node->name;
      entry.size = node->size;

      if (node->type == Node::FileNode) {
         entry.type = FolderEntry::File;
      } else if (node->type == Node::FolderNode) {
         entry.type = FolderEntry::Folder;
      } else {
         entry.type = FolderEntry::Unknown;
      }

      return true;
   }

protected:
   Node *findNode(const Path &path)
   {
      auto node = reinterpret_cast<Node *>(&mRoot);

      for (auto dir : path) {
         if (!node || node->type != Node::FolderNode) {
            return nullptr;
         }

         // Skip root directory
         if (node == &mRoot && dir.compare("/") == 0) {
            continue;
         }

         auto folder = reinterpret_cast<Folder *>(node);
         node = folder->findChild(dir);
      }

      return node;
   }

   Node *createPath(const Path &path)
   {
      auto node = reinterpret_cast<Node *>(&mRoot);

      for (auto dir : path) {
         if (!node || node->type != Node::FolderNode) {
            return nullptr;
         }

         // Skip root directory
         if (node == &mRoot && dir.compare("/") == 0) {
            continue;
         }

         auto folder = reinterpret_cast<Folder *>(node);
         node = folder->addFolder(dir);
      }

      return node;
   }

private:
   VirtualFolder mRoot;
};

} // namespace fs
