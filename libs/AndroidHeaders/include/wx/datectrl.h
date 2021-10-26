//#include <wx/textctrl.h>

#define wxDatePickerCtrlNameStr wxT("datectrl")

// wxDatePickerCtrl styles
enum
{
    // default style on this platform, either wxDP_SPIN or wxDP_DROPDOWN
    wxDP_DEFAULT = 0,

    // a spin control-like date picker (not supported in generic version)
    wxDP_SPIN = 1,

    // a combobox-like date picker (not supported in mac version)
    wxDP_DROPDOWN = 2,

    // always show century in the default date display (otherwise it depends on
    // the system date format which may include the century or not)
    wxDP_SHOWCENTURY = 4,

    // allow not having any valid date in the control (by default it always has
    // some date, today initially if no valid date specified in ctor)
    wxDP_ALLOWNONE = 8
};

class WXDLLIMPEXP_ADV wxDatePickerCtrl : public wxTextCtrl
{
public:
    // creating the control
//    wxDatePickerCtrl() { Init(); }
    virtual ~wxDatePickerCtrl() {}
    wxDatePickerCtrl(wxWindow *parent,
                     wxWindowID id,
                     const wxDateTime& date = wxDefaultDateTime,
                     const wxPoint& pos = wxDefaultPosition,
                     const wxSize& size = wxDefaultSize,
                     long style = wxDP_DEFAULT | wxDP_SHOWCENTURY,
                     const wxValidator& validator = wxDefaultValidator,
                     const wxString& name = wxDatePickerCtrlNameStr)
        : wxTextCtrl(parent, id, date.IsValid() ? date.Format() :"N/A", pos, size, 0, validator, name)
    {}
    // wxDatePickerCtrl methods
    void SetValue(const wxDateTime& date) { wxTextCtrl::SetValue(date.FormatISODate()); }
    wxDateTime GetDateCtrlValue() const { wxDateTime dt; dt.ParseISODate(wxTextCtrl::GetValue()); return dt; }

    bool GetRange(wxDateTime *dt1, wxDateTime *dt2) const {return true;}
    void SetRange(const wxDateTime &dt1, const wxDateTime &dt2) {}

    bool SetDateRange(const wxDateTime& lowerdate = wxDefaultDateTime,
                      const wxDateTime& upperdate = wxDefaultDateTime) {return true;}
};
