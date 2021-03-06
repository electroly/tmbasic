#pragma once

#include "../common.h"
#include "compiler/SourceProgram.h"

namespace tmbasic {

class DesignerWindow : public TWindow {
   public:
    DesignerWindow(const TRect& r, compiler::SourceMember* member, std::function<void()> onEdited);
    void handleEvent(TEvent& event) override;
    uint16_t getHelpCtx() override;
    void close() override;
    TPalette& getPalette() const override;
    void setState(uint16_t aState, bool enable) override;

   private:
    void onTimerTick();
    void updateTitle();
    void openPropertiesDialog();

    compiler::SourceMember* _member;
    std::function<void()> _onEdited;
};

}  // namespace tmbasic
