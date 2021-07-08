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


Bugfixes
--------


Improvements
############

- :ref:The ANSTO Bilby loader `LoadBBY <algm-LoadBBY>` logs the occurence of invalid events detected in the file as a warning.
- The ISIS SANS threading has been switched to use Python native threading. This provides users with much clearer error messages
  if something goes wrong, and improves tool compatibility for future development.


:ref:`Release 6.2.0 <v6.2.0>`
