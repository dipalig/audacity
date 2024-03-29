/**********************************************************************

  Audacity: A Digital Audio Editor

  FileHistory.h

  Leland Lucius

**********************************************************************/

#ifndef __AUDACITY_WIDGETS_FILEHISTORY__
#define __AUDACITY_WIDGETS_FILEHISTORY__

#include <wx/defs.h>
#include <wx/choice.h>
#include <wx/dynarray.h>
#include <wx/event.h>
#include <wx/grid.h>
#include <wx/string.h>
#include <wx/window.h>

class AUDACITY_DLL_API FileHistory
{
 public:
   FileHistory(int maxfiles = 9, wxWindowID idbase = wxID_FILE);
   virtual ~FileHistory();

   void AddFileToHistory(const wxString & file, bool update = true);
   void RemoveFileFromHistory(size_t i, bool update = true);
   void Clear();
   void UseMenu(wxMenu *menu);
   void RemoveMenu(wxMenu *menu);
   void Load(wxConfigBase& config, const wxString & group);
   void Save(wxConfigBase& config, const wxString & group);

   void AddFilesToMenu();
   void AddFilesToMenu(wxMenu *menu);

   size_t GetCount();
   wxString GetHistoryFile(size_t i) const;

 private:
   int mMaxFiles;
   wxWindowID mIDBase;

   wxArrayPtrVoid mMenus;
   wxArrayString mHistory;

};

#endif

// Indentation settings for Vim and Emacs and unique identifier for Arch, a
// version control system. Please do not modify past this point.
//
// Local Variables:
// c-basic-offset: 3
// indent-tabs-mode: nil
// End:
//
// vim: et sts=3 sw=3
// arch-tag: 94f72c32-970b-4f4e-bbf3-3880fce7b965
