#include "nachostabla.h"
#include "bitmap.h"
#include "system.h"

NachosOpenFilesTable::NachosOpenFilesTable() {
    maxFiles = 20;
    openFilesMap = new BitMap(maxFiles);
    openFiles = new OpenFile*[maxFiles];
    threadsPerFile = new int[maxFiles];

    for (int i = 0; i < maxFiles; i++) {
        openFiles[i] = NULL;
        threadsPerFile[i] = 0;
    }

   std::cout << "Tabla de archivos abiertos inicializada con " << maxFiles << " slots.\n";
}

NachosOpenFilesTable::~NachosOpenFilesTable() {
    for (int i = 0; i < maxFiles; i++) {
        if (openFiles[i] != NULL)
            delete openFiles[i];
    }

    delete[] openFiles;
    delete openFilesMap;
    delete[] threadsPerFile;

    DEBUG(dbgFileSys, "NachosOpenFilesTable destruida\n");
}

int NachosOpenFilesTable::Open(OpenFile* file) {
  int freeSlot = openFilesMap->Find();
  if (freeSlot == -1) return -1;

  openFiles[freeSlot] = file;
  threadsPerFile[freeSlot] = 1;
  return freeSlot;
}

int NachosOpenFilesTable::Close(int nachosHandle) {
    if (nachosHandle < 0 || nachosHandle >= maxFiles || !openFilesMap->Test(nachosHandle)) {
        return -1;
    }

    threadsPerFile[nachosHandle]--;
    if (threadsPerFile[nachosHandle] == 0) {
        delete openFiles[nachosHandle];
        openFiles[nachosHandle] = NULL;
        openFilesMap->Clear(nachosHandle);
    }

    return 0;
}

OpenFile* NachosOpenFilesTable::getOpenFile(int nachosHandle) {
    if (nachosHandle < 0 || nachosHandle >= maxFiles || !openFilesMap->Test(nachosHandle)) {
        return NULL;
    }
    return openFiles[nachosHandle];
}

bool NachosOpenFilesTable::isOpened(int nachosHandle) const {
    return (nachosHandle >= 0 && nachosHandle < maxFiles && openFilesMap->Test(nachosHandle));
}

void NachosOpenFilesTable::addThread(int nachosHandle) {
    if (nachosHandle >= 0 && nachosHandle < maxFiles && openFilesMap->Test(nachosHandle)) {
        threadsPerFile[nachosHandle]++;
    }
}

void NachosOpenFilesTable::delThread() {
    for (int i = 0; i < maxFiles; i++) {
        if (openFiles[i] != NULL && threadsPerFile[i] > 0) {
            threadsPerFile[i]--;
            if (threadsPerFile[i] == 0) {
                delete openFiles[i];
                openFiles[i] = NULL;
                openFilesMap->Clear(i);
            }
        }
    }
}

void NachosOpenFilesTable::Print() const {
    std::cout << "Tabla de archivos abiertos:\n";
    for (int i = 0; i < maxFiles; i++) {
        if (openFilesMap->Test(i)) {
            std::cout << "Slot " << i << ": archivo abierto, threads = " << threadsPerFile[i] << std::endl;
        }
    }
}
int NachosOpenFilesTable::AddSocket(int sockfd) {
    for (int i = 2; i < MAX_OPEN_FILES; ++i) { // empieza en 2 para evitar stdin y stdout
        if (openFiles[i] == NULL) {
            openFiles[i] = (OpenFile*)sockfd; // guardás el int como si fuera puntero (⚠️)
            isSocket[i] = true;               // marcás que este fd es un socket
            return i;
        }
    }
    return -1; // sin espacio
}