# Project Reunion

Project Reunion provides functionality that supports all your apps on Windows.

## Roadmap

We're following a [roadmap](docs/roadmap.md) that gets us to building a v1.0 of Reunion exploring
some basic common features.  You can help by filing issues for features you'd like to see!

## Documentation

* [Roadmap](docs/roadmap.md) - where we're going
* [High-level overview](docs/overview.md) - why we're doing this, how you can use it
* [Contributor/developer Guide](docs/developer-guide.md) - how to contribute to Reunion
* [App Author Guide](docs/consumer-guide.md) - how to use Reunion in your app
* [Implementation/API design Guide](docs/implementation-guide.md) - how to define and implement
  Reunion components.

## Features

### Benefits

Windows and Reunion loves all your apps.

The Reunion Framework helps you use modern features of Windows across all your apps, across all your
users. You'll get access to modern features of the Windows Application Platform at your own pace.
Improve power use and uphold privacy requirements in your existing apps.  Update your app to use
APIs that automatically switch to fill-in support on older versions of Windows.  Get access to
more of Windows from your UWPs. Connect to notifications from your Win32 applications.

1. Reunion stays up to date automatically as new versions are available, without recompilation or
updates to your app.

2. Reunion APIs are backwards-compatible across a range of Windows versions, or provide a clear
message to you that an API might not be supported.

3. Reunion works for packaged, unpackaged, UWP, Desktop Bridge, and bring-your-own identity apps.

4. Reunion helps you incrementally adopt powerful Windows features like AppContainer, Desktop
Bridge, Identity, and more.

### Version Support

At this time, Reunion supports Windows `TargetPlatformVersion` >= 10.0.18362 (Windows 10 1809) or
greater. 

Your app's users can be on any of the following supported Windows 10 versions:

* Windows Insider Previews
* May 2019 Update (18362 aka "19H1")
* October 2018 Update (17763 aka "Redstone 5")
* April 2018 Update (17134 aka "Redstone 4")
* Fall Creators Update (16299 aka "Redstone 3")

When features have partial implementations on older versions of Windows they will have an
[`IsSupported`](docs/developer-guide.md) method indicating what level of support is present.

## Data/Telemetry

This project collects usage data and sends it to Microsoft to help improve our products and services. See the [privacy statement](privacy.md) for more details.

For more information on telemetry implementation see the [developer guide](docs/developer-guide.md#Telemetry).

# Quick Start

## Using Reunion in Your Apps

There are [multiple samples available](samples/samples.md). Briefly the steps are:

1. [Add a reference](docs/consumer-guide.md#Packages) to the Microsoft.Reunion package
2. [Update your code](docs/consumer-guide.md#UpdateYourApp) to use Reunion functionality
3. [Deploy your app](docs/consumer-guide.md#DeployingReunion) with Reunion

## Requesting Reunion Functionality

File a [new issue!](https://github.com/microsoft/ProjectReunion/issues/new/choose) Tell us what problem you're
trying to solve, how you've tried to solve it so far, and what would be the ideal solution for your app.  Bonus
points if there's a gist or existing repo we can look at with you.  There's [a fuller description](docs/expanding-reunion.md)
of how this works, how you & the community can help, and more available.

## Contributing to Union

First, read the [Contributing](#Contributing) section.  Then:

1. Create a fork of this repo, clone it
1. Start Visual Studio 2019 on [ProjectReunion.sln](./ProjectReunion.sln)
1. [Modify the code](docs/developer-guide.md#UpdatingReunion) you'd like to see changed
1. [Add tests](docs/developer-guide.md#Testing) for your fix or functionality
1. [Create a pull request](docs/developer-guide.md#PullRequest) from your fork to this repo

# Contributing

This project welcomes contributions and suggestions.  Most contributions require you to agree to a
Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us
the rights to use your contribution. For details, visit https://cla.opensource.microsoft.com.

When you submit a pull request, a CLA bot will automatically determine whether you need to provide
a CLA and decorate the PR appropriately (e.g., status check, comment). Simply follow the instructions
provided by the bot. You will only need to do this once across all repos using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or
contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

# Legal Notices

Microsoft and any contributors grant you a license to the Microsoft documentation and other content
in this repository under the [Creative Commons Attribution 4.0 International Public License](https://creativecommons.org/licenses/by/4.0/legalcode),
see the [LICENSE](LICENSE) file, and grant you a license to any code in the repository under the [MIT License](https://opensource.org/licenses/MIT), see the
[LICENSE-CODE](LICENSE-CODE) file.

Microsoft, Windows, Microsoft Azure and/or other Microsoft products and services referenced in the documentation
may be either trademarks or registered trademarks of Microsoft in the United States and/or other countries.
The licenses for this project do not grant you rights to use any Microsoft names, logos, or trademarks.
Microsoft's general trademark guidelines can be found at http://go.microsoft.com/fwlink/?LinkID=254653.

Privacy information can be found at https://privacy.microsoft.com/en-us/

Microsoft and any contributors reserve all other rights, whether under their respective copyrights, patents,
or trademarks, whether by implication, estoppel or otherwise.
