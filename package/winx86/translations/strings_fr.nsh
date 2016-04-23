#
# Project: LibrePilot
# NSIS header file for LibrePilot GCS
# The OpenPilot Team, http://www.openpilot.org, Copyright (C) 2010-2011.
# The LibrePilot Team, http://www.librepilot.org, Copyright (C) 2015-2016.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
#

;
; Translation file for ${LANG_FRENCH}
;

;--------------------------------
; Installer section descriptions

  LangString DESC_InSecCore ${LANG_FRENCH} "Composants GCS principaux (executable et librairies)."
  LangString DESC_InSecPlugins ${LANG_FRENCH} "Plugins GCS (fournissent la plupart des fonctions)."
  LangString DESC_InSecLibs ${LANG_FRENCH} "Librairies tierces GCS (fournissent des fonctions supplémentaires)."
  LangString DESC_InSecResources ${LANG_FRENCH} "Ressources GCS (diagrammes, cadrans, modèles 3d, PFD)."
  LangString DESC_InSecSounds ${LANG_FRENCH} "Fichiers son GCS (pour les notifications sonores)."
  LangString DESC_InSecLocalization ${LANG_FRENCH} "Fichiers de localisation (langues supportées)."
  LangString DESC_InSecFirmware ${LANG_FRENCH} "LibrePilot firmware binaries."
  LangString DESC_InSecUtilities ${LANG_FRENCH} "LibrePilot utilities (Matlab log parser)."
  LangString DESC_InSecDrivers ${LANG_FRENCH} "OpenPilot hardware driver files (optional OpenPilot CDC driver)."
  LangString DESC_InSecInstallDrivers ${LANG_FRENCH} "Optional OpenPilot CDC driver (virtual USB COM port)."
  LangString DESC_InSecInstallOpenGL ${LANG_FRENCH} "Optional OpenGL32.dll for old video cards."
  LangString DESC_InSecAeroSimRC ${LANG_FRENCH} "AeroSimRC plugin files with sample configuration."
  LangString DESC_InSecShortcuts ${LANG_FRENCH} "Installer les raccourcis dans le menu démarrer."

;--------------------------------
; Uninstaller section descriptions

  LangString DESC_UnSecProgram ${LANG_FRENCH} "Application LibrePilot GCS et ses composants."
  LangString DESC_UnSecCache ${LANG_FRENCH} "Données en cache LibrePilot GCS."
  LangString DESC_UnSecConfig ${LANG_FRENCH} "Fichiers de configuration LibrePilot GCS."
