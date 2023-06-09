---
tip: translate by openai@2023-06-09 09:29:18
...

This document is a declaration of software quality for the `rcl_lifecycle` package, based on the guidelines in [REP-2004](https://www.ros.org/reps/rep-2004.html).

> 这份文件是基于[REP-2004](https://www.ros.org/reps/rep-2004.html)中的指导原则，对`rcl_lifecycle`包的软件质量声明。

# `rcl_lifecycle` Quality Declaration

The package `rcl_lifecycle` claims to be in the **Quality Level 1** category when it is used with a **Quality Level 1** middleware.

Below are the rationales, notes, and caveats for this claim, organized by each requirement listed in the [Package Quality Categories in REP-2004](https://www.ros.org/reps/rep-2004.html).

> 以下是根据 [REP-2004 中列出的包质量类别]（https://www.ros.org/reps/rep-2004.html）中列出的每个要求组织的这一主张的理由、注释和注意事项。

## Version Policy [1]

### Version Scheme [1.i]

`rcl_lifecycle` uses `semver` according to the recommendation for ROS Core packages in the [ROS 2 Developer Guide](https://docs.ros.org/en/rolling/Contributing/Developer-Guide.html#versioning).

> `rcl_lifecycle` 根据[ROS 2 开发者指南](https://docs.ros.org/en/rolling/Contributing/Developer-Guide.html#versioning)中 ROS 核心包的建议使用`semver`。

### Version Stability [1.ii]

`rcl_lifecycle` is at a stable version, i.e. `>= 1.0.0`.
The current version can be found in its [package.xml](package.xml), and its change history can be found in its [CHANGELOG](CHANGELOG.rst).

### Public API Declaration [1.iii]

All symbols in the installed headers are considered part of the public API.

All installed headers are in the `include` directory of the package, headers in any other folders are not installed and considered private.

### API Stability Within a Released ROS Distribution [1.iv]/[1.vi]

`rcl_lifecycle` will not break public API within a released ROS distribution, i.e. no major releases once the ROS distribution is released.

### ABI Stability Within a Released ROS Distribution [1.v]/[1.vi]

`rcl_lifecycle` contains C and C++ code and therefore must be concerned with ABI stability, and will maintain ABI stability within a ROS distribution.

> `rcl_lifecycle` 包含 C 和 C++代码，因此必须考虑 ABI 稳定性，并且将在 ROS 发行版中维护 ABI 稳定性。

## Change Control Process [2]

`rcl_lifecycle` follows the recommended guidelines for ROS Core packages in the [ROS 2 Developer Guide](https://docs.ros.org/en/rolling/Contributing/Developer-Guide.html#quality-practices).

> `rcl_lifecycle`遵循[ROS 2 开发指南](https://docs.ros.org/en/rolling/Contributing/Developer-Guide.html#quality-practices)中 ROS 核心包的推荐指南。

### Change Requests [2.i]

This package requires that all changes occur through a pull request.

### Contributor Origin [2.ii]

This package uses DCO as its confirmation of contributor origin policy. More information can be found in [CONTRIBUTING](../CONTRIBUTING.md).

### Peer Review Policy [2.iii]

Following the recommended guidelines for ROS Core packages, all pull requests must have at least 1 peer review.

### Continuous Integration [2.iv]

All pull requests must pass CI on all [tier 1 platforms](https://www.ros.org/reps/rep-2000.html#support-tiers).

### Documentation Policy [2.v]

All pull requests must resolve related documentation changes before merging.

## Documentation [3]

### Feature Documentation [3.i]

`rcl_lifecycle` has feature documentation describing lifecycle nodes.
It is [hosted](https://design.ros2.org/articles/node_lifecycle.html).

### Public API Documentation [3.ii]

Most of `rcl_lifecycle` has embedded API documentation. It is not yet hosted publicly.

### License [3.iii]

The license for `rcl_lifecycle` is Apache 2.0, and a summary is in each source file, the type is declared in the [package.xml](package.xml) manifest file, and a full copy of the license is in the [LICENSE](../LICENSE) file.

> `rcl_lifecycle`的许可证是 Apache 2.0，每个源文件中都有摘要，类型声明在[package.xml](package.xml)清单文件中，完整的许可证复制在[LICENSE](../LICENSE)文件中。

There is an automated test which runs a linter that ensures each file has a license statement.

The most recent test results can be found [here](https://ci.ros2.org/view/nightly/job/nightly_linux_release/lastBuild/testReport/rcl_lifecycle/copyright/).

> 最新的测试结果可以在[这里](https://ci.ros2.org/view/nightly/job/nightly_linux_release/lastBuild/testReport/rcl_lifecycle/copyright/)找到。

### Copyright Statements [3.iv]

The copyright holders each provide a statement of copyright in each source code file in `rcl_lifecycle`.

There is an automated test which runs a linter that ensures each file has at least one copyright statement.

The results of the test can be found [here](https://ci.ros2.org/view/nightly/job/nightly_linux_release/lastBuild/testReport/rcl_lifecycle/copyright/).

> 结果可以在[这里](https://ci.ros2.org/view/nightly/job/nightly_linux_release/lastBuild/testReport/rcl_lifecycle/copyright/)找到。

## Testing [4]

### Feature Testing [4.i]

`rcl_lifecycle` has feature tests, which test for proper node state transitions.
The tests are located in the [test](test) subdirectory.
New features are required to have tests before being added.
Currently nightly test results can be seen here:

- [linux-aarch64_release](https://ci.ros2.org/view/nightly/job/nightly_linux-aarch64_release/lastBuild/testReport/rcl_lifecycle/)
- [linux_release](https://ci.ros2.org/view/nightly/job/nightly_linux_release/lastBuild/testReport/rcl_lifecycle/)
- [mac_osx_release](https://ci.ros2.org/view/nightly/job/nightly_osx_release/lastBuild/testReport/rcl_lifecycle/)
- [windows_release](https://ci.ros2.org/view/nightly/job/nightly_win_rel/lastBuild/testReport/rcl_lifecycle/)

### Public API Testing [4.ii]

Each part of the public API has tests, and new additions or changes to the public API require tests before being added. The tests aim to cover both typical usage and corner cases, but are quantified by contributing to code coverage.

> 每个公共 API 部分都有测试，新增加或更改公共 API 需要在添加前进行测试。测试旨在覆盖典型用法和边界情况，但是通过贡献代码覆盖率来量化。

### Coverage [4.iii]

`rcl_lifecycle` follows the recommendations for ROS Core packages in the [ROS 2 Developer Guide](https://docs.ros.org/en/rolling/Contributing/Developer-Guide.html#code-coverage), and opts to use line coverage instead of branch coverage.

> `rcl_lifecycle`遵循[ROS 2 开发者指南](https://docs.ros.org/en/rolling/Contributing/Developer-Guide.html#code-coverage)中 ROS 核心包的建议，选择使用行覆盖率而不是分支覆盖率。

This includes:

- tracking and reporting line coverage statistics
- no lines are manually skipped in coverage calculations

Changes are required to make a best effort to keep or increase coverage before being accepted, but decreases are allowed if properly justified and accepted by reviewers.

> 需要做出更改以尽最大努力保持或增加覆盖面，才能被接受，但如果经过合理的理由并得到审查人员的认可，也可以允许减少。

Current coverage statistics can be viewed [here](https://ci.ros2.org/job/nightly_linux_coverage/lastSuccessfulBuild/cobertura/src_ros2_rcl_rcl_lifecycle_src/). A description of how coverage statistics are calculated is summarized in this page ["ROS 2 Onboarding Guide"](https://docs.ros.org/en/rolling/Contributing/Developer-Guide.html#note-on-coverage-runs).

> 当前覆盖率统计可在[此处](https://ci.ros2.org/job/nightly_linux_coverage/lastSuccessfulBuild/cobertura/src_ros2_rcl_rcl_lifecycle_src/)查看。如何计算覆盖率统计的描述概括在此页面 ["ROS 2 上手指南"](https://docs.ros.org/en/rolling/Contributing/Developer-Guide.html#note-on-coverage-runs)中。

### Performance [4.iv]

`rcl_lifecycle` follows the recommendations for performance testing of C code in the [ROS 2 Developer Guide](https://docs.ros.org/en/rolling/Contributing/Developer-Guide.html#performance), and opts to do performance analysis on each release rather than each change.

> `rcl_lifecycle`遵循[ROS 2 开发者指南](https://docs.ros.org/en/rolling/Contributing/Developer-Guide.html#performance)中关于 C 代码性能测试的建议，并选择在每次发布时而不是每次更改时进行性能分析。

System level performance benchmarks that cover features of `rcl_lifecycle` can be found at:

- [Benchmarks](http://build.ros2.org/view/Rci/job/Rci__benchmark_ubuntu_focal_amd64/BenchmarkTable/)
- [Performance](http://build.ros2.org/view/Rci/job/Rci__nightly-performance_ubuntu_focal_amd64/lastCompletedBuild/)

Changes that introduce regressions in performance must be adequately justified in order to be accepted and merged.

### Linters and Static Analysis [4.v]

`rcl_lifecycle` uses and passes all the standard linters and static analysis tools for a C package as described in the [ROS 2 Developer Guide](https://docs.ros.org/en/rolling/Contributing/Developer-Guide.html#linters-and-static-analysis).

> `rcl_lifecycle`使用和传递[ROS 2 开发者指南](https://docs.ros.org/en/rolling/Contributing/Developer-Guide.html#linters-and-static-analysis)中描述的所有标准 linters 和静态分析工具来处理 C 包。

Results of the nightly linter tests can be found [here](https://ci.ros2.org/view/nightly/job/nightly_linux_release/lastBuild/testReport/rcl_lifecycle).

> 测试结果可以在[这里](https://ci.ros2.org/view/nightly/job/nightly_linux_release/lastBuild/testReport/rcl_lifecycle)找到。

## Dependencies [5]

Below are evaluations of each of `rcl_lifecycle`'s run-time and build-time dependencies that have been determined to influence the quality.

It has several "buildtool" dependencies, which do not affect the resulting quality of the package, because they do not contribute to the public library API.

> 它有几个“构建工具”依赖项，这些依赖项不会影响打包的质量，因为它们不会对公共库 API 有所贡献。

It also has several test dependencies, which do not affect the resulting quality of the package, because they are only used to build and run the test code.

> 它还有几个测试依赖项，这些不会影响包的最终质量，因为它们只用于构建和运行测试代码。

### Direct Runtime ROS Dependencies [5.i]/[5.ii]

`rcl_lifecycle` has the following runtime ROS dependencies:

#### `lifecycle_msgs`

`lifecycle_msgs` provides message and services for managing lifecycle nodes.

It is **Quality Level 1**, see its [Quality Declaration document](https://github.com/ros2/rcl_interfaces/blob/master/lifecycle_msgs/QUALITY_DECLARATION.md).

> 这是一级质量，请参见其[质量声明文件](https://github.com/ros2/rcl_interfaces/blob/master/lifecycle_msgs/QUALITY_DECLARATION.md)。

#### `rcl`

`rcl` is the ROS 2 client library in C.

It is **Quality Level 1**, see its [Quality Declaration document](../rcl/QUALITY_DECLARATION.md).

#### `rcutils`

`rcutils` provides commonly used functionality in C.

It is **Quality Level 1**, see its [Quality Declaration document](https://github.com/ros2/rcutils/blob/master/QUALITY_DECLARATION.md).

#### `rmw`

`rmw` is the ROS 2 middleware library.

It is **Quality Level 1**, see its [Quality Declaration document](https://github.com/ros2/rmw/blob/master/rmw/QUALITY_DECLARATION.md).

#### `rosidl_runtime_c`

`rosidl_runtime_c` provides runtime functionality for rosidl message and service interfaces.

It is **Quality Level 1**, see its [Quality Declaration document](https://github.com/ros2/rosidl/blob/master/rosidl_runtime_c/QUALITY_DECLARATION.md).

> 这是质量等级 1，请参阅其[质量声明文件](https://github.com/ros2/rosidl/blob/master/rosidl_runtime_c/QUALITY_DECLARATION.md)。

#### `tracetools`

The `tracetools` package provides utilities for instrumenting the code in `rcl_lifecycle` so that it may be traced for debugging and performance analysis.

> `tracetools` 包提供了用于对 `rcl_lifecycle` 中的代码进行跟踪以进行调试和性能分析的实用程序。

It is **Quality Level 1**, see its [Quality Declaration document](https://gitlab.com/ros-tracing/ros2_tracing/-/blob/master/tracetools/QUALITY_DECLARATION.md).

> 这是质量等级 1，请参见其[质量声明文件](https://gitlab.com/ros-tracing/ros2_tracing/-/blob/master/tracetools/QUALITY_DECLARATION.md)。

### Direct Runtime Non-ROS Dependencies [5.iii]

`rcl_lifecycle` does not have any runtime non-ROS dependencies.

## Platform Support [6]

`rcl_lifecycle` supports all of the tier 1 platforms as described in [REP-2000](https://www.ros.org/reps/rep-2000.html#support-tiers), and tests each change against all of them.

> `rcl_lifecycle`支持[REP-2000](https://www.ros.org/reps/rep-2000.html#support-tiers)中描述的所有一级平台，并对所有更改进行测试。

Currently nightly results can be seen here:

- [linux-aarch64_release](https://ci.ros2.org/view/nightly/job/nightly_linux-aarch64_release/lastBuild/testReport/rcl_lifecycle/)
- [linux_release](https://ci.ros2.org/view/nightly/job/nightly_linux_release/lastBuild/testReport/rcl_lifecycle/)
- [mac_osx_release](https://ci.ros2.org/view/nightly/job/nightly_osx_release/lastBuild/testReport/rcl_lifecycle/)
- [windows_release](https://ci.ros2.org/view/nightly/job/nightly_win_rel/lastBuild/testReport/rcl_lifecycle/)

# Security [7]

## Vulnerability Disclosure Policy [7.i]

This package conforms to the Vulnerability Disclosure Policy in [REP-2006](https://www.ros.org/reps/rep-2006.html).
