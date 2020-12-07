function MakeCursor(CanvasKit) {
    const linePaint = new CanvasKit.Paint();
    linePaint.setColor([0,0,1,1]);
    linePaint.setStyle(CanvasKit.PaintStyle.Stroke);
    linePaint.setStrokeWidth(2);
    linePaint.setAntiAlias(true);

    const pathPaint = new CanvasKit.Paint();
    pathPaint.setColor([0,0,1,0.25]);
    linePaint.setAntiAlias(true);

    return {
        _line_paint: linePaint,    // wrap in weak-ref so we can delete it?
        _path_paint: pathPaint,
        _x: 0,
        _top: 0,
        _bottom: 0,
        _path: null,            // only use x,top,bottom if path is null
        _draws_per_sec: 2,

        // pass 0 for no-draw, pass inf. for always on
        setBlinkRate: function(blinks_per_sec) {
            this._draws_per_sec = blinks_per_sec;
        },
        place: function(x, top, bottom) {
            this._x = x;
            this._top = top;
            this._bottom = bottom;

            this._path = null;
        },
        setPath: function(path) {
            this._path = path;
        },
        draw_before: function(canvas) {
            if (this._path) {
                canvas.drawPath(this._path, this._path_paint);
            }
        },
        draw_after: function(canvas) {
            if (this._path) {
                return;
            }
            if (Math.floor(Date.now() * this._draws_per_sec / 1000) & 1) {
                canvas.drawLine(this._x, this._top, this._x, this._bottom, this._line_paint);
            }
        },
    };
}

function MakeMouse() {
    return {
        _start_x: 0, _start_y: 0,
        _curr_x:  0,  _curr_y: 0,
        _active: false,

        isActive: function() {
            return this._active;
        },
        setDown: function(x, y) {
            this._start_x = this._curr_x = x;
            this._start_y = this._curr_y = y;
            this._active = true;
        },
        setMove: function(x, y) {
            this._curr_x = x;
            this._curr_y = y;
        },
        setUp: function(x, y) {
            this._curr_x = x;
            this._curr_y = y;
            this._active = false;
        },
        getPos: function(dx, dy) {
            return [ this._start_x + dx, this._start_y + dy, this._curr_x + dx, this._curr_y + dy ];
        },
    };
}

function runs_x_to_index(runs, x) {
    for (const r of runs) {
        for (let i = 1; i < r.offsets.length; i += 1) {
            if (x < r.positions[i*2]) {
                const mid = (r.positions[i*2-2] + r.positions[i*2]) * 0.5;
                if (x <= mid) {
                    return r.offsets[i-1];
                } else {
                    return r.offsets[i];
                }
            }
        }
    }
    const r = runs[runs.length-1];
    return r.offsets[r.offsets.length-1];
}

function lines_pos_to_index(lines, x, y) {
    if (y < lines[0].top) {
        return 0;
    }
    for (const l of lines) {
        if (y <= l.bottom) {
            return runs_x_to_index(l.runs, x);
        }
    }
    return lines[lines.length - 1].textRange.last + 1;
}

function runs_index_to_run(runs, index) {
    for (const r of runs) {
        if (index <= r.offsets[r.offsets.length-1]) {
            return r;
        }
    }
    return null;
}

function runs_index_to_x(runs, index) {
  const r = runs_index_to_run(runs, index);
  for (const i in r.offsets) {
      if (index == r.offsets[i]) {
          return r.positions[i*2];
      }
  }
  return null;
}

function lines_index_to_line_index(lines, index) {
  let i = 0;
  for (const l of lines) {
      if (index <= l.textRange.last) {
          return i;
      }
      i += 1;
  }
  return lines.length-1;
}

function lines_index_to_line(lines, index) {
  return lines[lines_index_to_line_index(lines, index)];
}

function lines_index_to_x(lines, index) {
  for (const l of lines) {
      if (index <= l.textRange.last) {
          return runs_index_to_x(l.runs, index);
      }
  }
}

function lines_indices_to_path(lines, a, b, width) {
    if (a == b) {
        return null;
    }
    if (a > b) { [a, b] = [b, a]; }

    const path = new CanvasKit.Path();
    const la = lines_index_to_line(lines, a);
    const lb = lines_index_to_line(lines, b);
    const ax = runs_index_to_x(la.runs, a);
    const bx = runs_index_to_x(lb.runs, b);
    if (la == lb) {
        path.addRect([ax, la.top, bx, la.bottom]);
    } else {
        path.addRect([ax, la.top, width, la.bottom]);
        path.addRect([0, lb.top, bx, lb.bottom]);
        if (la.bottom < lb.top) {
            path.addRect([0, la.bottom, width, lb.top]);   // extra lines inbetween
        }
    }
    return path;
}

function string_del(str, start, end) {
    return str.slice(0, start) + str.slice(end, str.length);
}

function make_default_paint() {
    const p = new CanvasKit.Paint();
    p.setAntiAlias(true);
    return p;
}

function MakeStyle(length) {
    return {
        _length: length,
        typeface: null,
        size: null,
        color: null,
        bold: null,
        italic: null,

        // returns true if we changed something affecting layout
        mergeFrom: function(src) {
            let layoutChanged = false;

            if (src.typeface && this.typeface !== src.typeface) {
                this.typeface = src.typeface;
                layoutChanged = true;
            }
            if (src.size && this.size !== src.size) {
                this.size = src.size;
                layoutChanged = true;
            }
            if (src.color)    { this.color  = src.color; }
            if (src.bold)     { this.bold   = src.bold; }
            if (src.italic)   { this.italic = src.italic; }

            return layoutChanged;
        }
    };
}

function MakeEditor(text, style, cursor, width) {
    const ed = {
        _text: text,
        _lines: null,
        _cursor: cursor,
        _width: width,
        _index: { start: 0, end: 0 },
        _styles: null,
        // drawing
        _X: 0,
        _Y: 0,
        _paint: make_default_paint(),
        _font: new CanvasKit.Font(style.typeface),

        getLines: function() { return this._lines; },

        width: function() {
            return this._width;
        },
        height: function() {
            return this._lines[this._lines.length-1].bottom;
        },
        bounds: function() {
            return [this._X, this._Y, this._X + this.width(), this._Y + this.height()];
        },
        setXY: function(x, y) {
            this._X = x;
            this._Y = y;
        },

        setIndex: function(i) {
            this._index.start = this._index.end = i;
            const l = lines_index_to_line(this._lines, i);
            const x = runs_index_to_x(l.runs, i);
            this._cursor.place(x, l.top, l.bottom);
        },
        setIndices: function(a, b) {
            if (a > b) { [a, b] = [b, a]; }
            this._index.start = a;
            this._index.end = b;
            this._cursor.setPath(lines_indices_to_path(this._lines, a, b, this._width));
        },
        moveDX: function(dx) {
            let index;
            if (this._index.start == this._index.end) {
                // just adjust and pin
                index = Math.max(Math.min(this._index.start + dx, this._text.length), 0);
            } else {
                // 'deselect' the region, and turn it into just a single index
                index = dx < 0 ? this._index.start : this._index.end;
            }
            this.setIndex(index);
        },
        moveDY: function(dy) {
            let index = (dy < 0) ? this._index.start : this._index.end;
            const i = lines_index_to_line_index(this._lines, index);
            if (dy < 0 && i == 0) {
                index = 0;
            } else if (dy > 0 && i == this._lines.length - 1) {
                index = this._text.length;
            } else {
                const x = runs_index_to_x(this._lines[i].runs, index);
                // todo: statefully track "original" x when an up/down sequence started,
                //       so we can avoid drift.
                index = runs_x_to_index(this._lines[i+dy].runs, x);
            }
            this.setIndex(index);
        },

        _validateStyles: function() {
            let len = 0;
            for (const s of this._styles) {
                len += s._length;
            }
            if (len !== this._text.length) {
                console.log('bad style lengths', this._text.length, blocks);
                throw "";
            }
        },
        _validateBlocks: function(blocks) {
            let len = 0;
            for (const b of blocks) {
                len += b.length;
            }
            if (len !== this._text.length) {
                console.log('bad block lengths', this._text.length, blocks);
                throw "";
            }
        },

        _buildLines: function() {
            const blocks = [];
            let block = null;
            for (const s of this._styles) {
                if (!block || (block.typeface === s.typeface && block.size === s.size)) {
                    if (!block) {
                        block = { length: 0, typeface: s.typeface, size: s.size };
                    }
                    block.length += s._length;
                } else {
                    blocks.push(block);
                    block = { length: s._length, typeface: s.typeface, size: s.size };
                }
            }
            blocks.push(block);
            this._validateBlocks(blocks);

            console.log('new blocks', blocks);
            this._lines = CanvasKit.ParagraphBuilder.ShapeText(this._text, blocks, this._width);
        },

        deleteSelection: function() {
            let start = this._index.start;
            if (start == this._index.end) {
                if (start > 0) {
                    this._text = string_del(this._text, start - 1, start);
                    start -= 1;
                }
            } else {
                this._text = string_del(this._text,  start, this._index.end);
            }
            this._buildLines();
            this.setIndex(start);
        },
        insert: function(charcode) {
            if (this._index.start != this._index.end) {
                this.deleteSelection();
            }
            const index = this._index.start;
            this._text = this._text.slice(0, index) + charcode + this._text.slice(index);
            this._buildLines();
            this.setIndex(index + 1);
        },

        draw: function(canvas) {
            canvas.save();
            canvas.translate(this._X, this._Y);
            this._cursor.draw_before(canvas);
            for (const l of this._lines) {
                for (let r of l.runs) {
    //              this._font.setTypeface(r.typeface); // r.typeface is always null (for now)
                    this._font.setSize(r.size);
                    canvas.drawGlyphs(r.glyphs, r.positions, 0, 0, this._font, this._paint);
                }
            }
            this._cursor.draw_after(canvas);
            canvas.restore();
        },

        // Styling

        applyStyleToRange: function(style, start, end) {
            if (start > end) { [start, end] = [end, start]; }
            if (start < 0 || end > this._text.length) {
                throw "style selection out of range";
            }
            if (start === end) {
                return;
            }

            console.log('trying to apply', style, start, end);
            let i;
            for (i = 0; i < this._styles.length; ++i) {
                if (start <= this._styles[i]._length) {
                    break;
                }
                start -= this._styles[i]._length;
                end -= this._styles[i]._length;
            }

            let s = this._styles[i];
            // do we need to fission off a clean subset for the head of s?
            if (start > 0) {
                const ns = Object.assign({}, s);
                s._length = start;
                ns._length -= start;
                console.log('initial splice', i, start, s._length, ns._length);
                i += 1;
                this._styles.splice(i, 0, ns);
                end -= start;
                // we don't use start any more
            }
            // merge into any/all whole styles we overlap
            let layoutChanged = false;
            while (end >= this._styles[i]._length) {
                console.log('whole run merging for style index', i)
                layoutChanged |= this._styles[i].mergeFrom(style);
                end -= this._styles[i]._length;
                i += 1;
                if (end == 0) {
                    break;
                }
            }
            // do we partially cover the last run
            if (end > 0) {
                s = this._styles[i];
                const ns = Object.assign({}, s);    // the new first half
                ns._length = end;
                s._length -= end;                   // trim the (unchanged) tail
                console.log('merging tail', i, ns._length, s._length);
                layoutChanged |= ns.mergeFrom(style);
                this._styles.splice(i, 0, ns);
            }

            this._validateStyles();
            console.log('after applying styles', this._styles);

            if (layoutChanged) {
                this._buildLines();
            }
        },
        applyStyleToSelection: function(style) {
            applyStyleToRange(style, this._index.start, this._index.end);
        },
    };

    const s = MakeStyle(ed._text.length);
    s.mergeFrom(style);
    ed._styles = [ s ];
    ed._buildLines();
    return ed;
}