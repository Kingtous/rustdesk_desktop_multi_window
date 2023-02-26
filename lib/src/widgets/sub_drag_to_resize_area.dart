import 'dart:io';

import 'package:flutter/material.dart';

import '../window_controller.dart';

/// From window manager
enum SubWindowResizeEdge {
  top,
  left,
  right,
  bottom,
  topLeft,
  bottomLeft,
  topRight,
  bottomRight
}

class SubWindowDragToResizeArea extends StatelessWidget {
  final int windowId;
  final Widget child;
  final double resizeEdgeSize;
  final Color resizeEdgeColor;
  final EdgeInsets resizeEdgeMargin;
  final List<SubWindowResizeEdge>? enableResizeEdges;

  const SubWindowDragToResizeArea({
    Key? key,
    required this.windowId,
    required this.child,
    this.resizeEdgeColor = Colors.transparent,
    this.resizeEdgeSize = 8,
    this.resizeEdgeMargin = EdgeInsets.zero,
    this.enableResizeEdges,
  }) : super(key: key);

  Widget _buildDragToResizeEdge(
    SubWindowResizeEdge resizeEdge, {
    MouseCursor cursor = SystemMouseCursors.basic,
    double? width,
    double? height,
  }) {
    if (enableResizeEdges != null && !enableResizeEdges!.contains(resizeEdge))
      return Container();
    return Container(
      width: width,
      height: height,
      color: this.resizeEdgeColor,
      child: MouseRegion(
        cursor: cursor,
        child: GestureDetector(
          onPanStart: (_) =>
              WindowController.fromWindowId(windowId).startResizing(resizeEdge),
          onDoubleTap: () => (Platform.isWindows &&
                  (resizeEdge == SubWindowResizeEdge.top ||
                      resizeEdge == SubWindowResizeEdge.bottom))
              ? WindowController.fromWindowId(windowId).maximize()
              : null,
        ),
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Stack(
      children: <Widget>[
        Platform.isLinux ? 
        Container(
          margin: EdgeInsets.all(6.0),
          child: child,
        ): child,
        Positioned(
          child: Container(
            margin: resizeEdgeMargin,
            child: Column(
              children: [
                Row(
                  children: [
                    _buildDragToResizeEdge(
                      SubWindowResizeEdge.topLeft,
                      cursor: SystemMouseCursors.resizeUpLeft,
                      width: resizeEdgeSize,
                      height: resizeEdgeSize,
                    ),
                    Expanded(
                      flex: 1,
                      child: _buildDragToResizeEdge(
                        SubWindowResizeEdge.top,
                        cursor: SystemMouseCursors.resizeUp,
                        height: resizeEdgeSize,
                      ),
                    ),
                    _buildDragToResizeEdge(
                      SubWindowResizeEdge.topRight,
                      cursor: SystemMouseCursors.resizeUpRight,
                      width: resizeEdgeSize,
                      height: resizeEdgeSize,
                    ),
                  ],
                ),
                Expanded(
                  flex: 1,
                  child: Row(
                    children: [
                      _buildDragToResizeEdge(
                        SubWindowResizeEdge.left,
                        cursor: SystemMouseCursors.resizeLeft,
                        width: resizeEdgeSize,
                        height: double.infinity,
                      ),
                      Expanded(
                        flex: 1,
                        child: Container(),
                      ),
                      _buildDragToResizeEdge(
                        SubWindowResizeEdge.right,
                        cursor: SystemMouseCursors.resizeRight,
                        width: resizeEdgeSize,
                        height: double.infinity,
                      ),
                    ],
                  ),
                ),
                Row(
                  children: [
                    _buildDragToResizeEdge(
                      SubWindowResizeEdge.bottomLeft,
                      cursor: SystemMouseCursors.resizeDownLeft,
                      width: resizeEdgeSize,
                      height: resizeEdgeSize,
                    ),
                    Expanded(
                      flex: 1,
                      child: _buildDragToResizeEdge(
                        SubWindowResizeEdge.bottom,
                        cursor: SystemMouseCursors.resizeDown,
                        height: resizeEdgeSize,
                      ),
                    ),
                    _buildDragToResizeEdge(
                      SubWindowResizeEdge.bottomRight,
                      cursor: SystemMouseCursors.resizeDownRight,
                      width: resizeEdgeSize,
                      height: resizeEdgeSize,
                    ),
                  ],
                ),
              ],
            ),
          ),
        ),
      ],
    );
  }
}
