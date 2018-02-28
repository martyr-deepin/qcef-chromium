/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2013 Google Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "core/page/DragData.h"

#include "core/clipboard/DataObject.h"
#include "core/dom/DocumentFragment.h"
#include "core/dom/Range.h"
#include "core/editing/serializers/Serialization.h"
#include "core/frame/LocalFrame.h"
#include "platform/FileMetadata.h"
#include "platform/clipboard/ClipboardMimeTypes.h"
#include "platform/weborigin/KURL.h"
#include "platform/wtf/text/WTFString.h"

namespace blink {

DragData::DragData(DataObject* data,
                   const IntPoint& client_position,
                   const IntPoint& global_position,
                   DragOperation source_operation_mask,
                   DragApplicationFlags flags)
    : client_position_(client_position),
      global_position_(global_position),
      platform_drag_data_(data),
      dragging_source_operation_mask_(source_operation_mask),
      application_flags_(flags) {}

bool DragData::ContainsHTML() const {
  return platform_drag_data_->Types().Contains(kMimeTypeTextHTML);
}

bool DragData::ContainsURL(FilenameConversionPolicy filename_policy) const {
  return platform_drag_data_->Types().Contains(kMimeTypeTextURIList) ||
         (filename_policy == kConvertFilenames &&
          platform_drag_data_->ContainsFilenames());
}

String DragData::AsURL(FilenameConversionPolicy filename_policy,
                       String* title) const {
  String url;
  if (platform_drag_data_->Types().Contains(kMimeTypeTextURIList))
    platform_drag_data_->UrlAndTitle(url, title);
  else if (filename_policy == kConvertFilenames && ContainsFiles())
    url = FilePathToURL(platform_drag_data_->Filenames()[0]);
  return url;
}

bool DragData::ContainsFiles() const {
  return platform_drag_data_->ContainsFilenames();
}

int DragData::GetModifiers() const {
  return platform_drag_data_->GetModifiers();
}

void DragData::AsFilePaths(Vector<String>& result) const {
  const Vector<String>& filenames = platform_drag_data_->Filenames();
  for (size_t i = 0; i < filenames.size(); ++i) {
    if (!filenames[i].IsEmpty())
      result.push_back(filenames[i]);
  }
}

unsigned DragData::NumberOfFiles() const {
  return platform_drag_data_->Filenames().size();
}

bool DragData::ContainsPlainText() const {
  return platform_drag_data_->Types().Contains(kMimeTypeTextPlain);
}

String DragData::AsPlainText() const {
  return platform_drag_data_->GetData(kMimeTypeTextPlain);
}

bool DragData::CanSmartReplace() const {
  // Mimic the situations in which mac allows drag&drop to do a smart replace.
  // This is allowed whenever the drag data contains a 'range' (ie.,
  // ClipboardWin::writeRange is called). For example, dragging a link
  // should not result in a space being added.
  return platform_drag_data_->Types().Contains(kMimeTypeTextPlain) &&
         !platform_drag_data_->Types().Contains(kMimeTypeTextURIList);
}

bool DragData::ContainsCompatibleContent() const {
  return ContainsPlainText() || ContainsURL() || ContainsHTML() ||
         ContainsFiles();
}

DocumentFragment* DragData::AsFragment(LocalFrame* frame) const {
  /*
     * Order is richest format first. On OSX this is:
     * * Web Archive
     * * Filenames
     * * HTML
     * * RTF
     * * TIFF
     * * PICT
     */

  if (ContainsFiles()) {
    // FIXME: Implement this. Should be pretty simple to make some HTML
    // and call createFragmentFromMarkup.
  }

  if (ContainsHTML()) {
    String html;
    KURL base_url;
    platform_drag_data_->HtmlAndBaseURL(html, base_url);
    DCHECK(frame->GetDocument());
    if (DocumentFragment* fragment =
            CreateFragmentFromMarkup(*frame->GetDocument(), html, base_url,
                                     kDisallowScriptingAndPluginContent))
      return fragment;
  }

  return nullptr;
}

String DragData::DroppedFileSystemId() const {
  return platform_drag_data_->FilesystemId();
}

}  // namespace blink