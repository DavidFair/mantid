============
SANS Changes
============

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

New
---

- :ref:`SANSILLIntegration <algm-SANSILLIntegration>` has new resolution calculation option alternative to Mildner-Carpenter based on fitting horizontal size of direct beam. The fitting is handled in :ref:`SANSILLReduction <algm-SANSILLReduction>` while processing beam.
- ISIS SANS GUI will automatically toggle between Can SAS and NXS Can SAS when switching between 1D and 2D reductions.
  If you have toggled any save options it will not update the selected methods until the interface is restarted to avoid interfering with the user's save selection.

Bugfixes
--------


Improvements
############

- :ref:The ANSTO Bilby loader `LoadBBY <algm-LoadBBY>` logs the occurence of invalid events detected in the file as a warning.

:ref:`Release 6.2.0 <v6.2.0>`
