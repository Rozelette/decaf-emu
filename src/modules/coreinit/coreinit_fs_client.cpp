#include <algorithm>
#include "coreinit_fs.h"
#include "coreinit_fs_client.h"
#include "utils/wfunc_call.h"

namespace coreinit
{

static std::vector<FSClient*>
gClients;

FSClient::FSClient()
{
   // Let's just ensure there is never a file handle 0 just in case it's not a valid handle
   mOpenFiles.push_back(nullptr);
}


FSFileHandle
FSClient::addOpenFile(fs::FileHandle *file)
{
   std::lock_guard<std::mutex> lock(mMutex);

   // Try use an existing slot
   for (auto i = 1; i < mOpenFiles.size(); ++i) {
      if (mOpenFiles[i] == nullptr) {
         mOpenFiles[i] = file;
         return i;
      }
   }

   // Add a new slot
   auto handle = static_cast<FSFileHandle>(mOpenFiles.size());
   mOpenFiles.push_back(file);
   return handle;
}


FSDirectoryHandle
FSClient::addOpenDirectory(fs::FolderHandle *folder)
{
   std::lock_guard<std::mutex> lock(mMutex);

   // Try use an existing slot
   for (auto i = 1; i < mOpenFolders.size(); ++i) {
      if (mOpenFolders[i] == nullptr) {
         mOpenFolders[i] = folder;
         return i;
      }
   }

   // Add a new slot
   auto handle = static_cast<FSFileHandle>(mOpenFolders.size());
   mOpenFolders.push_back(folder);
   return handle;
}


fs::FileHandle *
FSClient::getOpenFile(FSFileHandle handle)
{
   std::lock_guard<std::mutex> lock(mMutex);

   if (handle > mOpenFiles.size()) {
      return nullptr;
   }

   return mOpenFiles[handle];
}


fs::FolderHandle *
FSClient::getOpenDirectory(FSDirectoryHandle handle)
{
   std::lock_guard<std::mutex> lock(mMutex);

   if (handle > mOpenFolders.size()) {
      return nullptr;
   }

   return mOpenFolders[handle];
}


void
FSClient::removeOpenDirectory(FSDirectoryHandle handle)
{
   std::lock_guard<std::mutex> lock(mMutex);

   if (handle > mOpenFolders.size()) {
      return;
   } else {
      if (mOpenFolders[handle]) {
         mOpenFolders[handle]->close();
      }

      mOpenFolders[handle] = nullptr;
   }
}


void
FSClient::removeOpenFile(FSFileHandle handle)
{
   std::lock_guard<std::mutex> lock(mMutex);

   if (handle > mOpenFiles.size()) {
      return;
   } else {
      if (mOpenFiles[handle]) {
         mOpenFiles[handle]->close();
      }

      mOpenFiles[handle] = nullptr;
   }
}


void
FSClient::setLastError(FSError error)
{
   mLastError = error;
}


FSError
FSClient::getLastError()
{
   return mLastError;
}


FSStatus
FSAddClient(FSClient *client,
            uint32_t flags)
{
   return FSAddClientEx(client, 0, flags);
}


FSStatus
FSAddClientEx(FSClient *client,
              uint32_t unk,
              uint32_t flags)
{
   new(client) FSClient();
   gClients.push_back(client);
   return FSStatus::OK;
}


FSStatus
FSDelClient(FSClient *client,
            uint32_t flags)
{
   client->~FSClient();
   gClients.erase(std::remove(gClients.begin(), gClients.end(), client), gClients.end());
   return FSStatus::OK;
}


uint32_t
FSGetClientNum()
{
   return static_cast<uint32_t>(gClients.size());
}


FSVolumeState
FSGetVolumeState(FSClient *client)
{
   return FSVolumeState::Ready;
}


FSError
FSGetLastErrorCodeForViewer(FSClient *client)
{
   return client->getLastError();
}


void
FSSetStateChangeNotification(FSClient *client,
                             FSStateChangeInfo *info)
{
   // TODO: FSSetStateChangeNotification
}


namespace internal
{

void
doAsyncFileCallback(FSClient *client, FSCmdBlock *block, FSStatus result, FSAsyncData *asyncData)
{
   if (!asyncData->callback) {
      throw std::logic_error("Async file command with null callback");
   }

   // Immediately call async callback
   auto callback = static_cast<FSAsyncCallback>(asyncData->callback);
   callback(client, block, result, asyncData->param);
}

} // namespace internal

} // namespace coreinit
