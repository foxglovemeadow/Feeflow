from flask import Flask, render_template, request, redirect, url_for, session, flash, send_file
import sqlite3
import io
import qrcode

app = Flask(__name__)
app.secret_key = 'replace-this-with-a-secret-key'  # Change this to a strong secret key

def init_db():
    conn = sqlite3.connect('school_fees.db')
    c = conn.cursor()
    c.execute('''
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE NOT NULL,
            password TEXT NOT NULL
        )
    ''')
    c.execute('''
        CREATE TABLE IF NOT EXISTS students (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            class TEXT NOT NULL,
            total_fees REAL NOT NULL,
            amount_paid REAL NOT NULL DEFAULT 0
        )
    ''')
    conn.commit()
    conn.close()

def get_db_connection():
    conn = sqlite3.connect('school_fees.db')
    conn.row_factory = sqlite3.Row
    return conn

@app.route('/')
def index():
    if 'user_id' not in session:
        return redirect(url_for('login'))
    conn = get_db_connection()
    students = conn.execute('SELECT * FROM students').fetchall()
    conn.close()
    total_fees = sum(s['total_fees'] for s in students)
    total_paid = sum(s['amount_paid'] for s in students)
    total_balance = total_fees - total_paid
    return render_template('dashboard.html', students=students,
                           total_fees=total_fees, total_paid=total_paid, total_balance=total_balance)

@app.route('/signup', methods=['GET', 'POST'])
def signup():
    if request.method == 'POST':
        username = request.form['username'].strip()
        password = request.form['password'].strip()
        if not username or not password:
            flash('Username and password are required.')
            return redirect(url_for('signup'))
        conn = get_db_connection()
        try:
            conn.execute('INSERT INTO users (username, password) VALUES (?, ?)', (username, password))
            conn.commit()
            flash('Account created successfully! Please log in.')
            return redirect(url_for('login'))
        except sqlite3.IntegrityError:
            flash('Username already exists.')
            return redirect(url_for('signup'))
        finally:
            conn.close()
    return render_template('signup.html')

@app.route('/login', methods=['GET', 'POST'])
def login():
    if request.method == 'POST':
        username = request.form['username'].strip()
        password = request.form['password'].strip()
        conn = get_db_connection()
        user = conn.execute('SELECT * FROM users WHERE username = ? AND password = ?', (username, password)).fetchone()
        conn.close()
        if user:
            session['user_id'] = user['id']
            session['username'] = user['username']
            return redirect(url_for('index'))
        else:
            flash('Invalid username or password.')
            return redirect(url_for('login'))
    return render_template('login.html')

@app.route('/logout')
def logout():
    session.clear()
    return redirect(url_for('login'))

@app.route('/add', methods=['POST'])
def add_student():
    if 'user_id' not in session:
        return redirect(url_for('login'))
    name = request.form['name'].strip()
    class_name = request.form['class'].strip()
    total_fees = request.form['total_fees']
    amount_paid = request.form['amount_paid']
    try:
        total_fees = float(total_fees)
        amount_paid = float(amount_paid)
    except ValueError:
        flash('Total fees and amount paid must be numbers.')
        return redirect(url_for('index'))
    if not name or not class_name:
        flash('Name and class are required.')
        return redirect(url_for('index'))
    conn = get_db_connection()
    conn.execute('INSERT INTO students (name, class, total_fees, amount_paid) VALUES (?, ?, ?, ?)',
                 (name, class_name, total_fees, amount_paid))
    conn.commit()
    conn.close()
    flash('Student added successfully.')
    return redirect(url_for('index'))

@app.route('/update/<int:student_id>', methods=['POST'])
def update_payment(student_id):
    if 'user_id' not in session:
        return redirect(url_for('login'))
    payment = request.form.get('payment')
    try:
        payment = float(payment)
        if payment < 0:
            raise ValueError
    except (ValueError, TypeError):
        flash('Invalid payment amount.')
        return redirect(url_for('index'))
    conn = get_db_connection()
    student = conn.execute('SELECT * FROM students WHERE id = ?', (student_id,)).fetchone()
    if not student:
        conn.close()
        flash('Student not found.')
        return redirect(url_for('index'))
    new_amount_paid = student['amount_paid'] + payment
    if new_amount_paid > student['total_fees']:
        flash('Payment exceeds total fees.')
        conn.close()
        return redirect(url_for('index'))
    conn.execute('UPDATE students SET amount_paid = ? WHERE id = ?', (new_amount_paid, student_id))
    conn.commit()
    conn.close()
    flash('Payment updated successfully.')
    return redirect(url_for('index'))

@app.route('/qr')
def qr_code():
    app_url = "https://school-fees-tracker-2.onrender.com"  # Change to your live URL

    img = qrcode.make(app_url)

    buf = io.BytesIO()
    img.save(buf, format='PNG')
    buf.seek(0)

    return send_file(buf, mimetype='image/png')

if __name__ == '__main__':
    init_db()
    app.run(debug=True)
