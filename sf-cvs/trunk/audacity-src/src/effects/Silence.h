/**********************************************************************

  Audacity: A Digital Audio Editor

  Silence.h

  Dominic Mazzoni
  
  An effect for the "Generator" menu to add silence.

**********************************************************************/

#ifndef __AUDACITY_EFFECT_SILENCE__
#define __AUDACITY_EFFECT_SILENCE__

#include "Effect.h"

#include <wx/defs.h>
#include <wx/intl.h>

class EffectSilence:public Effect {

 public:
   EffectSilence() {}

   virtual wxString GetEffectName() {
      return wxString(_("Silence"));
   }

   virtual wxString GetEffectAction() {
      return wxString(_("Generating Silence"));
   }

   // Useful only after PromptUser values have been set. 
   virtual wxString GetEffectDescription() { 
      return wxString(_("Applied effect: Generate Silence")); 
   } 

   virtual int GetEffectFlags() {
      return BUILTIN_EFFECT | INSERT_EFFECT;
   }

   virtual bool Process();

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
// arch-tag: 077860ae-1dc5-4aa0-a70d-dcbd27e92dd5

