#ifndef _WX_TIMECTRL_H_
#define _WX_TIMECTRL_H_

#define wxTimePickerCtrlNameStr wxS("timectrl")

// No special styles are currently defined for this control but still define a
// symbolic constant for the default style for consistency.
enum { wxTP_DEFAULT = 0 };

class WXDLLIMPEXP_ADV wxTimePickerCtrl : public wxTextCtrl {
public:
    // creating the control
    virtual ~wxTimePickerCtrl() { }
    wxTimePickerCtrl(wxWindow* parent, wxWindowID id,
        const wxDateTime& date = wxDefaultDateTime,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxDP_DEFAULT | wxDP_SHOWCENTURY,
        const wxValidator& validator = wxDefaultValidator,
        const wxString& name = wxTimePickerCtrlNameStr)
        : wxTextCtrl(parent, id, date.Format(), pos, size, 0, validator, name)
    {
    }
    // wxTimePickerCtrl methods
    void SetValue(const wxDateTime& date)
    {
        wxTextCtrl::SetValue(date.FormatISOTime());
    }
    wxDateTime GetTimeCtrlValue() const
    {
        wxDateTime dt;
        dt.ParseISOTime(wxTextCtrl::GetValue());
        return dt;
    }
};

#endif
