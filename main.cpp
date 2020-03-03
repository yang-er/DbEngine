#include <iostream>
#include "storage/buffer.h"
#include "catalog/catalog.h"
#include "syntax/parser.h"
#include "indexing/indexing.h"
#include "record/recording.h"

using namespace std;

int main()
{
    bufferManager.reset(new buffer_t);
    catalogManager.reset(new catalog_t);
    catalogManager->load();
    recordingManager.reset(new recording_t);
    indexManager.reset(new indexing);

    lexer mmp(cin);
    parser t(mmp);
    executor wtf;

    cout << "Welcome to TinyDb!" << endl << endl;
    cout << "! Note that keywords should be UPPERCASE." << endl;
    cout << "! Thanks for using." << endl << endl << "> ";

    t.process(&wtf, true);

    indexManager.reset();
    recordingManager.reset();
    catalogManager->store();
    catalogManager.reset();
    bufferManager.reset();
    return 0;
}