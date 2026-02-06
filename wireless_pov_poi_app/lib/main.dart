import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'model.dart';
import 'pages/welcome.dart';

void main() {
  runApp(const WirelessPOVPoiApp());
}

class WirelessPOVPoiApp extends StatelessWidget {
  const WirelessPOVPoiApp({super.key});

  @override
  Widget build(BuildContext context) {
    return ChangeNotifierProvider(
      create: (context) => Model(),
      child: MaterialApp(
        title: 'Wireless POV Poi',
        theme: ThemeData(
          colorScheme: ColorScheme.fromSeed(seedColor: Colors.blue),
          useMaterial3: true,
        ),
        home: const WelcomePage(),
      ),
    );
  }
}
