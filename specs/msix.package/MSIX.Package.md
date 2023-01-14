# 1. MSIX Package

This feature provides APIs to query packages and related information including identity, location,
display information and other properties of a package.

These APIs provide functionality similar to Windows.ApplicationModel.Package,
Windows.Management.Deployment.PackageManager.Find*() and appmodel.h but without their limitations.
This also provides the start and foundation of Undocked Deployment.

# 2. Background

MSIX packages on a system can be queried since Windows 8, but as MSIX has evolved so too have the
package query and management APIs and information. Existing APIs have been under perpetual
enhancement. Windows App SDK provides the opportunity to provide Undocked APIs with polyfill
implementations so enhancements are available to downlevel systems and not just the latest release
of Windows.

Microsoft-internal task [TODO](https://task.ms/TODO)

This is the spec for proposal [Undocked Package APIs #TODO](https://github.com/microsoft/WindowsAppSDK/issues/TODO).

# 3. Description

TODO

# 4. Examples

Samples illustrating the Package APIs

- [Sample 1](sample-1.md) - [TODO](sample-1.md)

# 5. Remarks

TODO

## 5.1. API Overview

TODO

# 6. API Details

## 6.1. WinRT API

```c# (but really MIDL3)
namespace Microsoft.Windows.ApplicationModel
{
    [contractversion(1)]
    apicontract PackageContract{};

    [contract(PackageContract, 1)]
    runtimeclass PackageStatus
    {
        // Equivalent to a W.AM.PackageStatus object
        PackageStatus(Windows.ApplicationModel.PackageStatus packageStatus);

        Boolean IsOK { get; };

        Boolean NotAvailable { get; };
        Boolean PackageOffline { get; };
        Boolean DataOffline { get; };
        Boolean Disabled { get; };

        Boolean NeedsRemediation { get; };
        Boolean LicenseIssue { get; };
        Boolean Modified { get; };
        Boolean Tampered { get; };
        Boolean DependencyIssue { get; };

        Boolean Servicing { get; };
        Boolean DeploymentInProgress { get; };
    }

    [contract(PackageContract, 1)]
    enum PackageSignatureKind
    {
        None       = 0,
        Developer  = 1,
        Enterprise = 2, //TODO Rename?
        Store      = 3,
        System     = 4,
    };

    [contract(PackageContract, 1)]
    runtimeclass FindPackageOptions
    {
        FindPackageOptions();

        Windows.System.User User;
        String PackageFullName;
        String PackageFamilyName;
        String PackageName;
        String PackagePublisher;
        String PackagePublisherId;
        Boolean IsMain;
        Boolean IsFramework;
        Boolean IsResource;
        Boolean IsOptional;
        Boolean IsOptionalInRelatedSet;
        Boolean IsOptionalNotInRelatedSet;
        Boolean IsBundle;
        Boolean IsProvisioned;
    };

    [contract(PackageContract, 1)]
    runtimeclass Package
    {
        // Equivalent to a W.AM.Package object
        Package(Windows.ApplicationModel.Package package);

        // Equivalent to W.AM.Package.Current
        static Package GetCurrent();

        PackageDisplayInfo DisplayInfo { get; };

        PackageLocation Location { get; };

        PackageIdentity Identity { get; };

        PackageGraph PackageGraph { get; };

        PackageInstallInfo Install { get; };    //TODO Rename InstallInfo? Rename all to *Info?

        PackageApplications Applications { get; };

        PackageContentGroups ContentGroups { get; };

        // Equivalent to W.M.D.PackageManager.FindPackage(String)
        static Package FindPackage(String packageFullName);

        // Equivalent to W.M.D.PackageManager.FindPackageForUser(String, String)
        static Package FindPackage(Windows.System.User user, String packageFullName);

        // Equivalent to W.M.D.PackageManager.FindPackages()
        static IVector<Package> FindPackages();

        // Equivalent to W.M.D.PackageManager.FindPackages(String)
        static IVector<Package> FindPackages(String packagFamilyName);

        // Equivalent to W.M.D.PackageManager.FindPackages(String, String)
        [method_name("FindPackagesByNameAndPublisher")]
        static IVector<Package> FindPackages(String packageName, String packagePublisher);

        // Equivalent to W.M.D.PackageManager.FindPackagesForUser(String)
        static IVector<Package> FindPackagesForUser(Windows.System.User user);

        // Equivalent to W.M.D.PackageManager.FindPackagesForUser(String, String)
        [method_name("FindPackagesByUserAndFamilyName")]
        static IVector<Package> FindPackagesForUser(Windows.System.User user, String packagFamilyName);

        // Equivalent to W.M.D.PackageManager.FindPackagesForUser(String, String, String)
        [method_name("FindPackagesByUserAndNameAndPublisher")]
        static IVector<Package> FindPackagesForUser(Windows.System.User user, String packageName, String packagePublisher);

        // Equivalent to W.M.D.PackageManager.FindPackagesForUserWithPackageTypes(String, PackageTypes)
        [method_name("FindPackagesByUserAndPackageType")]
        static IVector<Package> FindPackagesForUserWithPackageTypes(Windows.System.User user, Windows.Management.Deployment.PackageTypes types);

        // Equivalent to W.M.D.PackageManager.FindPackagesForUserWithPackageTypes(String, String, PackageTypes)
        [method_name("FindPackagesByUserAndFamilyNameAndPackageType")]
        static IVector<Package> FindPackagesForUserWithPackageTypes(Windows.System.User user, String packagFamilyName, Windows.Management.Deployment.PackageTypes types);

        // Equivalent to W.M.D.PackageManager.FindPackagesForUserWithPackageTypes(String, String, String, PackageTypes)
        [method_name("FindPackagesByUserAndNameAndPublisherAndPackageType")]
        static IVector<Package> FindPackagesForUserWithPackageTypes(Windows.System.User user, String packageName, String packagePublisher, Windows.Management.Deployment.PackageTypes types);

        // Equivalent to W.M.D.PackageManager.FindProvisionedPackages()
        static IVector<Package> FindProvisionedPackages();

        static Package FindPackageWithOptions(FindPackageOptions options);
        static IVector<Package> FindPackagesWithOptions(FindPackageOptions options);

        Windows.Management.Deployment.PackageTypes PackageType { get; };

        Boolean IsMain { get; };
        Boolean IsFramework { get; };
        Boolean IsResource { get; };
        Boolean IsOptional { get; };
        Boolean IsOptionalInRelatedSet { get; };
        Boolean IsBundle { get; };

        Boolean IsDevelopmentMode { get; };

        Boolean IsStub { get; };

        Boolean IsSigned { get; };
        Microsoft.Windows.ApplicationModel.PackageSignatureKind SignatureKind { get; };

        Microsoft.Windows.ApplicationModel.PackageStatus Status { get; };

        Windows.Foundation.IAsyncOperation<Windows.ApplicationModel.PackageUpdateAvailabilityResult> CheckUpdateAvailabilityAsync();

        Windows.Foundation.IAsyncOperation<Boolean> SetInUseAsync(Boolean inUse);

        Windows.Foundation.IAsyncOperation<Boolean> VerifyContentIntegrityAsync();

        Windows.ApplicationModel.Package W_AM_Package { get; };
    }

    [contract(PackageContract, 1)]
    runtimeclass PackageDisplayInfo
    {
        PackageDisplayInfo(Microsoft.Windows.ApplicationModel.Package package);

        String DisplayName { get; };
        String PublisherDisplayName { get; };
        String Description { get; };

        String LogoFilename { get; };
        Windows.Foundation.Uri LogoUri { get; };
        Windows.Storage.Streams.RandomAccessStreamReference GetLogoStream(Windows.Foundation.Size size);
        //TODO LogoImage() returning the image as byte[]? Image? ???
    }

    [contract(PackageContract, 1)]
    runtimeclass PackageLocation
    {
        PackageLocation(Microsoft.Windows.ApplicationModel.Package package);

        // Alias for .EffectivePath
        String Path { get; };

        // Alias for .EffectiveLocation
        Windows.Storage.StorageFolder Location { get; };

        String InstalledPath { get; };
        String MutablePath { get; };
        String EffectivePath { get; };
        String EffectiveExternalPath { get; };
        String UserEffectiveExternalPath { get; };
        String MachineEffectiveExternalPath { get; };

        Windows.Storage.StorageFolder InstalledLocation { get; };
        Windows.Storage.StorageFolder MutableLocation { get; };
        Windows.Storage.StorageFolder EffectiveLocation { get; };
        Windows.Storage.StorageFolder EffectiveExternalLocation { get; };
        Windows.Storage.StorageFolder UserEffectiveExternalLocation { get; };
        Windows.Storage.StorageFolder MachineEffectiveExternalLocation { get; };
    }

    [contract(PackageContract, 1)]
    runtimeclass PackageFamilyIdentity
    {
        PackageFamilyIdentity();
        PackageFamilyIdentity(String name, String publisherId);

        String Name;
        String PublisherId;
    }

    [contract(PackageContract, 1)]
    runtimeclass PackageIdentity
    {
        PackageIdentity(Microsoft.Windows.ApplicationModel.Package package);

        String Name { get; };
        Windows.ApplicationModel.PackageVersion Version { get; };
        Windows.System.ProcessorArchitecture Architecture { get; };
        String ResouceId { get; };
        String Publisher { get; };
        String PublisherId { get; };

        String PackageFullName { get; };
        String PackageFamilyName { get; };

        // Equivalent to PackageFullNameFromId(...,publisher...) in appmodel.h
        static String FormatPackageFullName(String packagename, Windows.ApplicationModel.PackageVersion version, Windows.System.ProcessorArchitecture architecture, String resourceId, String publisher);

        // Equivalent to PackageFullNameFromId(...,publisherid...) in appmodel.h
        static String FormatPackageFullNameGivenPublisherId(String packagename, Windows.ApplicationModel.PackageVersion version, Windows.System.ProcessorArchitecture architecture, String resourceId, String publisherId);

        // Equivalent to PackageIdFromFullName() in appmodel.h
        static PackageIdentity ParsePackageFullName(String packageFullName);

        // Equivalent to PackageFamilyNameFromFullName() in appmodel.h
        static String FormatPackageFamilyName(String packageFullName);

        // Equivalent to PackageFamilyNameFromId(...publisher...) in appmodel.h
        static String FormatPackageFamilyName(String packagename, String publisher);

        // Equivalent to PackageFamilyNameFromId(...publisherid...) in appmodel.h
        static String FormatPackageFamilyNameGivenPublisherId(String packagename, String publisherId);

        // Equivalent to PackageNameAndPublisherIdFromFamilyName() in appmodel.h
        static PackageFamilyIdentity ParsePackageFamilyName(String packageFamilyName);

        // Equivalent to VerifyPackageId(...publisher...) in appmodel.h
        static Boolean VerifyPackageId(String packagename, Windows.ApplicationModel.PackageVersion version, Windows.System.ProcessorArchitecture architecture, String resourceId, String publisher);

        // Equivalent to VerifyPackageId(...publisherid...) in appmodel.h
        static Boolean VerifyPackageIdGivenPublisherId(String packagename, Windows.ApplicationModel.PackageVersion version, Windows.System.ProcessorArchitecture architecture, String resourceId, String publisherId);

        // Equivalent to VerifyPackageFullName() in appmodel.h
        static Boolean VerifyPackageFullName(String packageFullName);

        // Equivalent to VerifyFamilyName() in appmodel.h
        static Boolean VerifyPackageFamilyName(String packageFullName);
    }

    [contract(PackageContract, 1)]
    enum PackageRelationship
    {
        Dependencies = 0,
        Dependents,
        All
    };

    [contract(PackageContract, 1)]
    runtimeclass FindRelatedPackagesOptions
    {
        FindRelatedPackagesOptions(PackageRelationship Relationship);

        PackageRelationship Relationship;
        Boolean IncludeFrameworks;
        Boolean IncludeHostRuntimes;
        Boolean IncludeOptionals;
        Boolean IncludeResources;
    };

    [contract(PackageContract, 1)]
    runtimeclass PackageGraph
    {
        PackageGraph(Microsoft.Windows.ApplicationModel.Package package);

        IVector<Package> FindRelatedPackages();

        IVector<Package> FindRelatedPackages(FindRelatedPackagesOptions options);
    }

    [contract(PackageContract, 1)]
    runtimeclass PackageInstallInfo
    {
        PackageInstallInfo(Microsoft.Windows.ApplicationModel.Package package);

        Windows.Foundation.DateTime WhenFirstRegisteredForUser { get; };

        // Equivalent to W.AM.Package.InstalledDate
        Windows.Foundation.DateTime WhenLastRegisteredForUser { get; };

        // Equivalent to W.AM.Package.AppInstallerInfo
        Windows.ApplicationModel.AppInstallerInfo GetAppInstallerInfo();

        // Equivalent to W.AM.Package.SourceUriSchemeName
        String SourceUriSchemeName { get; };
    }

    [contract(PackageContract, 1)]
    runtimeclass PackageApplications
    {
        PackageApplications(Microsoft.Windows.ApplicationModel.Package package);

        IVector<Windows.ApplicationModel.Core.AppListEntry> GetAppListEntries();
        Windows.Foundation.IAsyncOperation<IVector<Windows.ApplicationModel.Core.AppListEntry> > GetAppListEntriesAsync();
    }

    [contract(PackageContract, 1)]
    runtimeclass PackageContentGroups
    {
        PackageContentGroups(Microsoft.Windows.ApplicationModel.Package package);

        Windows.Foundation.IAsyncOperation<Windows.ApplicationModel.PackageContentGroup> GetContentGroupAsync(String name);
        Windows.Foundation.IAsyncOperation<IVector<Windows.ApplicationModel.PackageContentGroup> > GetContentGroupsAsync();

        Windows.Foundation.IAsyncOperation<IVector<Windows.ApplicationModel.PackageContentGroup> > StageContentGroups();
    }
}
```
