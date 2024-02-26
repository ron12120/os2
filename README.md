# OS2

***מטלה 2 מערכות הפעלה:***<br /> <br />
בנינו Makefile רקורסיבי שמקמפל את כל הקבצים בתקיות על מנת להפעיל אותו יש להשתמש בפקודה make all ועל מנת למחוק את קובצי ההרצה יש לכתוב את הפקודה make clean.
**דוגמת הרצת מייקפיל:**   <br />
<br />
![WhatsApp Image 2024-02-25 at 17 26 40](https://github.com/yuvit256/OS2/assets/76705730/37e128ef-e6dc-498d-aeb6-a6aa0b4fed89)
<br />
<br />
נציין כי בדקנו את השרת והלקוח עבור כל סוגי הקבצים ואכן כולם עבדו פרט לקובץ pdf שגם הוא נשלח ומתקבל כמו שצריך אך מסיבה כלשהי לא נבנה כמו שצריך, דיבאגנו ומצאנו כי הקובץ בינארי מגיע בשלומתו ולא הבנו מדוע אינו בונה קובץ תקין.
<br />
**חלק 1:**<br />
בחלק זה בנינו שרת פשוט:<br /><br />
על מנת להריץ את התוכנית server צריך להריץ בטרמינל:   <server <full_path_server/. 
<br />
**דוגמת הרצה שרת:**   <br />

![WhatsApp Image 2024-02-25 at 17 26 40 (1)](https://github.com/yuvit256/OS2/assets/76705730/02ff7baf-92a9-4f35-b5bf-e445d491ce42)

<br />

השרת מקבל שני סוגים של פקודות get ו post.
<br />
אנחנו משתמשים בלקוח של סעיף ב על מנת לבדוק את השרת, הלקוח יודע לעבוד עם פעולות GET ו POST של קבצים וגם את הדרישות של סעיף ב' אותם נתאר בהמשך.
<br />
דוגמה להרצה של לקוח עם פקודת POST:
<br />

![WhatsApp Image 2024-02-25 at 17 28 27](https://github.com/yuvit256/OS2/assets/76705730/4f06b51a-c4f2-44bb-ad71-4a9821d9dac9)
<br />
<br />
דוגמה להרצה של לקוח עם פקודת GET

<br />

![WhatsApp Image 2024-02-25 at 17 26 59 (2)](https://github.com/yuvit256/OS2/assets/76705730/84380ea3-65ff-4b6a-b844-bc87e611f16c)

<br />

![WhatsApp Image 2024-02-25 at 17 27 31](https://github.com/yuvit256/OS2/assets/76705730/f125de1c-d81c-421e-9ae9-d24db364a5d7)


<br />

כאשר אנו כותבים לתוך קובץ השרת נועל אותו ואי אפשר להוריד אותו:

<br />

![WhatsApp Image 2024-02-25 at 17 28 45 (1)](https://github.com/yuvit256/OS2/assets/76705730/9af2dfbb-2048-4de0-908b-9b180bfb729f)


<br /><br />
**חלק 2**
בחלק זה כתבנו לקוח (אותו לקוח שבדקנו איתו את חלק א'), הלקוח בודק אם קיבל קובץ מסוג list ואם כן מבצע פעול GET עבור כל קובץ בlist דוגמה לקובץ list:


<br />

![WhatsApp Image 2024-02-25 at 17 32 35](https://github.com/yuvit256/OS2/assets/76705730/547d496d-0cf1-4dae-aa97-c4f01a36f94e)


<br />


דוגמה להרצת הלקוח עם קובץ הlist:

<br />

![WhatsApp Image 2024-02-25 at 17 32 14](https://github.com/yuvit256/OS2/assets/76705730/7036530e-88f8-4898-ab89-91cc7f35e6bb)


<br />

פלט ההרצה:
<br />

![WhatsApp Image 2024-02-25 at 17 33 11](https://github.com/yuvit256/OS2/assets/76705730/ddd5a809-0b60-4552-acf5-3ae2c6250d96)

<br />


פלט הרצת השרת:
<br />

![image](https://github.com/yuvit256/OS2/assets/76705730/a47c3e45-7fff-4206-87e4-c8a46c06450e)


<br />


  
  
 
 



