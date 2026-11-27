#pragma once
#include "document/Document.h"

#include <AIS_InteractiveObject.hxx>
#include <AIS_Shape.hxx>

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace hcad::app {

class OcctViewWidget;

// A selected element expressed in persistent, document-level terms.
struct SelectionRef {
  std::string featureId; // feature that owns the displayed body
  std::string reference; // persistent sub-shape name, or empty for whole object
};

// Mirrors the document into AIS presentations. Rebuilt after every recompute.
class SceneSync {
public:
  explicit SceneSync(OcctViewWidget* view) : view_(view) {}

  void rebuild(const doc::Document& doc);

  // Current viewer selection converted into persistent references.
  std::vector<SelectionRef> currentSelection(const doc::Document& doc) const;
  // Re-selects persistent references after a recompute. Returns the number of
  // references that could be resolved.
  int restoreSelection(const doc::Document& doc, const std::vector<SelectionRef>& refs);

  const std::string* featureOf(const Handle(AIS_InteractiveObject)& obj) const;
  Handle(AIS_Shape) bodyPresentation(const std::string& featureId) const;

private:
  OcctViewWidget* view_;
  std::map<const AIS_InteractiveObject*, std::string> owners_;
  std::map<std::string, Handle(AIS_Shape)> bodies_;
};

} // namespace hcad::app
