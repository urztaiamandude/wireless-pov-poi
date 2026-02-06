import 'dart:math';
import 'package:flutter/foundation.dart' show kIsWeb;
import 'dbimage.dart';
import 'package:sqflite/sqflite.dart';
import 'package:sqflite_common/sqlite_api.dart';
import 'package:sqflite_common_ffi/sqflite_ffi.dart';
import 'package:sqflite_common_ffi_web/sqflite_ffi_web.dart';
import 'dart:async';
import 'package:path/path.dart';
import 'package:image/image.dart' as img;
import 'package:flutter/services.dart';
import 'package:flutter/widgets.dart';
import 'package:tuple/tuple.dart';
import '../config.dart';

class PatternDB {
  late Future<Database> databaseFuture;
  List<Tuple2<Widget, DBImage>>? inMemoryCache;

  PatternDB(){
    getDB();
  }

  Future<void> getDB() async {
    if (kIsWeb) {
      databaseFactory = databaseFactoryFfiWeb;
    } else {
      sqfliteFfiInit();
      databaseFactory = databaseFactoryFfi;
    }
    databaseFuture = openDatabase(
      join(await getDatabasesPath(), 'wireless_pov_patterns.db'),
      onCreate: (db, version) {
        return db.execute(
          'CREATE TABLE images(id INTEGER PRIMARY KEY, height INTEGER, count INTEGER, bytes BLOB)',
        );
      },
      version: 1,
    );

    List<DBImage> images = await getDBImages();
    if (images.isEmpty) {
      print("DB Empty, inserting default patterns");
      for (int i = 1; i <= 10; i++) {
        try {
          ByteData fileBytes = await rootBundle.load("patterns/pattern$i.bmp");
          Uint8List bytesList = fileBytes.buffer.asUint8List(fileBytes.offsetInBytes, fileBytes.lengthInBytes);
          img.Image image = img.decodeBmp(bytesList)!;

          List<int> imageBytes = List.empty(growable: true);
          for (var w = 0; w < image.width; w++) {
            for (var h = 0; h < image.height; h++) {
              var pixel = image.getPixel(w, h);
              imageBytes.add(pixel.r.toInt());
              imageBytes.add(pixel.g.toInt());
              imageBytes.add(pixel.b.toInt());
            }
          }

          var pattern = DBImage(
            id: null,
            height: image.height,
            count: image.width,
            bytes: Uint8List.fromList(imageBytes),
          );

          await insertImage(pattern);
        } catch (e) {
          print("Failed to load pattern$i.bmp: $e");
        }
      }
    }
  }

  Future<void> insertImage(DBImage image) async {
    // Validate pattern dimensions
    if (image.height > WirelessPOVConfig.MAX_PATTERN_HEIGHT) {
      throw Exception("Pattern height must be ≤${WirelessPOVConfig.MAX_PATTERN_HEIGHT} pixels (found: ${image.height})");
    }
    
    if (image.count > WirelessPOVConfig.MAX_PATTERN_WIDTH) {
      throw Exception("Pattern width must be ≤${WirelessPOVConfig.MAX_PATTERN_WIDTH} pixels (found: ${image.count})");
    }
    
    if (image.count * image.height > WirelessPOVConfig.MAX_PATTERN_PIXELS) {
      throw Exception("Pattern too large: ${image.count}×${image.height} = ${image.count * image.height} pixels (max: ${WirelessPOVConfig.MAX_PATTERN_PIXELS})");
    }
    
    // Validate RGB data length
    int expectedBytes = image.count * image.height * 3;
    if (image.bytes.length != expectedBytes) {
      throw Exception("Invalid RGB data length: expected $expectedBytes bytes, got ${image.bytes.length}");
    }
    
    final db = await databaseFuture;
    await db.insert(
      'images',
      image.toMap(),
      conflictAlgorithm: ConflictAlgorithm.replace,
    );
    clearInMemoryCache();
  }

  Future<void> deleteImage(int id) async {
    final db = await databaseFuture;

    await db.delete(
      'images',
      where: 'id = ?',
      whereArgs: [id],
    );
    clearInMemoryCache();
  }

  Future<List<DBImage>> getDBImages() async {
    final db = await databaseFuture;

    final List<Map<String, dynamic>> maps = await db.query('images');

    return List.generate(maps.length, (i) {
      return DBImage(
        id: maps[i]['id'],
        height: maps[i]['height'],
        count: maps[i]['count'],
        bytes: maps[i]['bytes'],
      );
    });
  }

  Future<DBImage> getDBImage(int id) async {
    final db = await databaseFuture;

    final List<Map<String, dynamic>> maps = await db.query('images', where: "id = ?", whereArgs: [id]);

    return DBImage(
      id: maps[0]['id'],
      height: maps[0]['height'],
      count: maps[0]['count'],
      bytes: maps[0]['bytes'],
    );
  }

  Future<void> invertImage(int id) async {
    var image = await getDBImage(id);
    List<int> inverted = List.empty(growable: true);
    for(int column = 0; column < image.count; column++){
      int offset = column * image.height * 3;
      for(int pixel = image.height - 1; pixel >= 0; pixel--){
        inverted.add(image.bytes[offset + (pixel * 3) + 0]);
        inverted.add(image.bytes[offset + (pixel * 3) + 1]);
        inverted.add(image.bytes[offset + (pixel * 3) + 2]);
      }
    }
    deleteImage(id);
    insertImage(DBImage(id: image.id, height: image.height, count: image.count, bytes: Uint8List.fromList(inverted)));
    clearInMemoryCache();
  }

  Future<void> reverseImage(int id) async {
    var image = await getDBImage(id);
    List<int> reversed = List.empty(growable: true);
    for(int column = image.count - 1; column >= 0; column--){
      int offset = column * image.height * 3;
      for(int pixel = 0; pixel < image.height; pixel++){
        reversed.add(image.bytes[offset + (pixel * 3) + 0]);
        reversed.add(image.bytes[offset + (pixel * 3) + 1]);
        reversed.add(image.bytes[offset + (pixel * 3) + 2]);
      }
    }
    deleteImage(id);
    insertImage(DBImage(id: image.id, height: image.height, count: image.count, bytes: Uint8List.fromList(reversed)));
    clearInMemoryCache();
  }

  void clearInMemoryCache() {
    inMemoryCache = null;
  }

  Future<List<Tuple2<Widget, DBImage>>> getImages(BuildContext context) async {
    if (inMemoryCache != null) {
      return inMemoryCache!;
    }

    List<DBImage> dbImages = await getDBImages();
    List<Tuple2<Widget, DBImage>> widgetImagePairs = [];

    for (var dbImage in dbImages) {
      Widget imageWidget = CustomPaint(
        painter: PatternPainter(dbImage),
        size: Size(200, 80),
      );
      widgetImagePairs.add(Tuple2(imageWidget, dbImage));
    }

    inMemoryCache = widgetImagePairs;
    return widgetImagePairs;
  }
}

class PatternPainter extends CustomPainter {
  final DBImage image;

  PatternPainter(this.image);

  @override
  void paint(Canvas canvas, Size size) {
    double pixelWidth = size.width / image.count;
    double pixelHeight = size.height / image.height;

    for (int col = 0; col < image.count; col++) {
      for (int row = 0; row < image.height; row++) {
        int offset = (col * image.height + row) * 3;
        int r = image.bytes[offset];
        int g = image.bytes[offset + 1];
        int b = image.bytes[offset + 2];

        Paint paint = Paint()..color = Color.fromARGB(255, r, g, b);
        canvas.drawRect(
          Rect.fromLTWH(col * pixelWidth, row * pixelHeight, pixelWidth, pixelHeight),
          paint,
        );
      }
    }
  }

  @override
  bool shouldRepaint(covariant CustomPainter oldDelegate) {
    return false;
  }
}
