FileOpenPicker API
===

This spec details the `Microsoft.Windows.Storage.Pickers.FileOpenPicker` API in the Windows App SDK, enabling desktop applications such as WinUI 3 apps to present a user interface for selecting files, integrating seamlessly across Windows platforms.

# Background

The current OS [FileOpenPicker](https://learn.microsoft.com/en-us/uwp/api/windows.storage.pickers.fileopenpicker) API doesn't support running as administrator.

The new `Microsoft.Windows.Storage.Pickers.FileOpenPicker` API allows applications to prompt the user to select one or more files to open. It provides a consistent and familiar interface for browsing and selecting files from the filesystem or other storage locations.

In desktop applications, integrating a file picker that works seamlessly can be challenging due to differences in application models and windowing systems. The `FileOpenPicker` in the Windows App SDK addresses these challenges by providing an API tailored for desktop applications, leveraging the `WindowId` to properly associate the picker UI with the application's window.

# Conceptual Pages

## Open a Single File (C++)

A C++ example that demonstrates how to use the `PickSingleFileAsync` method to allow the user to select a single file.

```cpp
// Include necessary headers
#include <winrt/Microsoft.Windows.Storage.Pickers.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Microsoft.UI.h>

// Get the WindowId for the window
winrt::Microsoft::UI::WindowId windowId = this->AppWindow().Id();

// Initialize FileOpenPicker with a window handle
winrt::Microsoft::Windows::Storage::Pickers::FileOpenPicker openPicker{ windowId };

// Configure file picker settings
openPicker.SuggestedStartLocation(winrt::Microsoft::Windows::Storage::Pickers::PickerLocationId::DocumentsLibrary);
openPicker.FileTypeFilter().Append(L".txt");
openPicker.FileTypeFilter().Append(L".docx");
openPicker.CommitButtonText(L"Open File");

// Pick a single file asynchronously
winrt::Windows::Storage::StorageFile file = co_await openPicker.PickSingleFileAsync();

if (file != nullptr)
{
    // File was selected, proceed with processing the file
    auto fileName = file.Name();
    // TODO: Add code to process the file
}
else
{
    // No file was selected, or the operation was canceled
}
```

## Open Multiple Files (C#)

A C# example that demonstrates how to allow the user to select multiple files using the `PickMultipleFilesAsync` method.

```csharp
using Microsoft.Windows.Storage.Pickers;
using Microsoft.UI;
using Windows.Storage;

// Get the WindowId for the window
Microsoft.UI.WindowId windowId = this.AppWindow.Id;

// Initialize FileOpenPicker with a window handle
FileOpenPicker openPicker = new FileOpenPicker(windowId);

// Configure file picker settings
openPicker.SuggestedStartLocation = PickerLocationId.PicturesLibrary;
openPicker.FileTypeFilter.Add(".jpg");
openPicker.FileTypeFilter.Add(".png");
openPicker.FileTypeFilter.Add(".gif");
openPicker.CommitButtonText = "Select Images";

// Pick multiple files asynchronously
var files = await openPicker.PickMultipleFilesAsync();

if (files.Count > 0)
{
    // Files were selected, proceed with processing
    foreach (StorageFile file in files)
    {
        var fileName = file.Name;
        // TODO: Add code to process each file
    }
}
else
{
    // No files were selected, or the operation was canceled
}
```

# FileOpenPicker Class

Provides a UI for users to select files. Supports picking single or multiple files.

## Properties

| Name                   | Description                                                        | Type                            |
|------------------------|--------------------------------------------------------------------|---------------------------------|
| **ViewMode**               | Gets or sets the view mode that the file picker uses to display items. | `PickerViewMode`                |
| **SettingsIdentifier**     | Gets or sets an identifier used to maintain the state of the file picker. | `string`                        |
| **SuggestedStartLocation** | Gets or sets the initial location where the file picker looks for files. | `PickerLocationId`              |
| **CommitButtonText**       | Gets or sets the label text of the commit button in the file picker UI. | `string`                        |
| **FileTypeFilter**         | Gets the collection of file types that the file open picker displays. | `IVector<string>`               |

## Methods

| Name                       | Description                                                                        | Parameters | Return Type                                 |
|----------------------------|------------------------------------------------------------------------------------|------------|---------------------------------------------|
| **PickSingleFileAsync()**    | Shows the file picker so that the user can pick one file.                          | None       | `IAsyncOperation<StorageFile>`              |
| **PickMultipleFilesAsync()** | Shows the file picker so that the user can pick multiple files.                    | None       | `IAsyncOperation<IVectorView<StorageFile>>` |

# Enumerations

## PickerViewMode Enum

Specifies the view mode that the file picker uses to display items.

| Name        | Description                              |
|-------------|------------------------------------------|
| **List**       | The items are displayed in a list.        |
| **Thumbnail**  | The items are displayed as thumbnails.    |

## PickerLocationId Enum

Specifies the initial location that the file picker suggests to the user.

| Name                      | Description                           |
|---------------------------|---------------------------------------|
| **DocumentsLibrary**          | The documents library.                 |
| **PicturesLibrary**           | The pictures library.                  |
| **MusicLibrary**              | The music library.                     |
| **VideosLibrary**             | The videos library.                    |
| **Desktop**                   | The desktop folder.                    |
| **Downloads**                 | The downloads folder.                  |
| **ComputerFolder**            | The "This PC" folder.                  |
| **RemovableDevices**          | The removable devices.                 |
| **Unspecified**               | Let Windows decide the best location.  |

# Usage Notes

- **ViewMode**: Determines whether items are displayed as thumbnails or in a list. Can be set to `PickerViewMode.List` or `PickerViewMode.Thumbnail`.

- **SettingsIdentifier**: Use this property to maintain the state of the picker (like last visited folder) across multiple invocations.

- **SuggestedStartLocation**: Suggests the initial location to start browsing.

- **CommitButtonText**: Allows customization of the text on the commit button (e.g., "Open", "Select").

- **FileTypeFilter**: You must add at least one file type extension (e.g., ".txt", ".jpg") to the `FileTypeFilter` collection for the picker to show any files.

- **WindowId**: The constructor of `FileOpenPicker` requires a `WindowId` to associate the picker with the application's window.

# Example Code Snippets

## Using FileTypeFilter

```cpp
// Initialize FileOpenPicker
winrt::Microsoft::Windows::Storage::Pickers::FileOpenPicker openPicker{ windowId };

// Clear existing filters (if any)
openPicker.FileTypeFilter().Clear();

// Add filters
openPicker.FileTypeFilter().Append(L".txt");
openPicker.FileTypeFilter().Append(L".docx");
openPicker.FileTypeFilter().Append(L".pdf");
```

## Customizing Commit Button Text

```csharp
FileOpenPicker openPicker = new FileOpenPicker(windowId);
openPicker.CommitButtonText = "Select Document";
```

# Remarks

- The file picker is a modal window; the code after `PickSingleFileAsync()` or `PickMultipleFilesAsync()` will not execute until the user has closed the picker.

- Always check if the returned `StorageFile` or the collection is not `null` or empty before proceeding.

- The `FileOpenPicker` API in the Windows App SDK is designed to work seamlessly with desktop applications, handling the necessary windowing and threading considerations.

# See Also

- [FileSavePicker](./FileSavePicker.md)
- [FolderPicker](./FolderPicker.md)
